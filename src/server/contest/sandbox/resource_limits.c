#include <stdio.h>
#include <string.h>
#include <stdlib.h> // malloc()
#include <math.h> // log10(), ceil()
#include <sys/types.h> // pid_t, SIGKILl, open()
#include <sys/stat.h> // S_IRWXU, open()
#include <sys/eventfd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h> // open()
#include <unistd.h> // close(), write()
#include <pthread.h>
#include <time.h> // nanosleep()
#include <semaphore.h>

#include "logger.h"
#include "resource_limits.h"
#include "terminate.h"

#define TIME_LIM_POLL_INTERVAL 100 // nanoseconds
#define NUM_TASKS_LIM_POLL_INTERVAL 100 // nanoseconds

typedef struct MemListenerPayload {
  int oomefd;
  int mocfd;
  int *exceeded;
  char *pid_dir;
  pid_t pid;
  const CgroupLocs *cg_locs;
  pthread_mutex_t *term_child_mutex;
} MemListenerPayload;

typedef struct MemListenerCleanupPayload {
  MemListenerPayload *mlp;
} MemListenerCleanupPayload;

typedef struct CpuTimeLimiterPayload {
  int *exceeded;
  const char *cpu_time;
  char *pid_dir;
  pid_t pid;
  const CgroupLocs *cg_locs;
  pthread_mutex_t *term_child_mutex;
} CpuTimeLimiterPayload;

typedef struct CpuTimeLimiterCleanupPayload {
  int fd;
  CpuTimeLimiterPayload *ctlp;
} CpuTimeLimiterCleanupPayload;

typedef struct NumTasksListenerPayload {
  int *exceeded;
  char *pid_dir;
  pid_t pid;
  const CgroupLocs *cg_locs;
  pthread_mutex_t *term_child_mutex;
} NumTasksListenerPayload;

typedef struct NumTasksListenerCleanupPayload {
  int fd;
  char *pids_events_path;
  NumTasksListenerPayload *ntlp;
} NumTasksListenerCleanupPayload;

// ------------------------- Helper Functions - Begin -----------------------

/*
  Creates the directory |cg|/|pid| and returns a pointer to the NTCS
  |cg|/|pid|

  Returns:
    NULL on error
    not NULL on success

  Resource residue (when return value != NULL):
    1 char malloc - 'pid_dir'
    1 directory created - |pid_dir|
*/
static char *createPidDir(const char *cg, pid_t pid) {

  char *pid_dir = getPidDir(cg, pid);
  // TODO: check what permissions are required
  if (mkdir(pid_dir, S_IRWXU) == -1) {
    printErr("mkdir failed: errno: %d", errno);
    free(pid_dir);
    return NULL;
  }
  return pid_dir;
}

/*
  Writes |str| to the file |dir|/|file_name|
*/
static int writeToFile(
  const char *dir, const char *file_name, const char *str) {

  char *path = malloc(sizeof(char) * (strlen(dir) + strlen(file_name) + 2));
  sprintf(path, "%s/%s", dir, file_name);
  int f = open(path, O_WRONLY);
  if (f == -1) {
    printErr("open failed: errno: %d", errno);
    free(path);
    return -1;
  }
  if (write(f, str, sizeof(char) * strlen(str)) == -1) {
    printErr("write failed: errno: %d", errno);
    free(path);
    return -1;
  }
  if (close(f) == -1) {
    printErr("close failed: errno: %d", errno);
    free(path);
    return -1;
  }
  free(path);
  return 0;
}

/*
  |pid| is written to |cg|/|pid|/tasks
*/
static int addToTasksFile(const char *cg, pid_t pid) {

  char *pid_dir = getPidDir(cg, pid);
  char *buf = malloc((int)ceil(log10(pid)) + 1);
  sprintf(buf, "%d", pid);
  if (writeToFile(pid_dir, "tasks", buf) == -1) {
    printErr("writeToFile failed");
    free(pid_dir);
    free(buf);
    return -1;
  }
  free(pid_dir);
  free(buf);
  return 0;
}

/*
  Keeps reading from file descriptor |fd| at intervals of |interval| nanoseconds

  Returns:
    0 if the read string is greater than |lim|
    -1 on error

    Note: The meaning of greater than depends on the value of 'type'. The read string
    is greater than |lim| if
      (for type == 1)
        the number represented by the read string is > number represented by 'lim'
      (for type != 1)
        the read string is lexicographically greater then the string 'lim'
*/
static int pollFileContent(
  int fd, const char *lim, long long interval, int type) {

  long long seconds = interval / 10e9;
  long long nanoseconds = interval - seconds;
  struct timespec req;
  req.tv_sec = seconds;
  req.tv_nsec = nanoseconds;

  int content_max_size = 20;
  char *content = malloc(sizeof(char) * content_max_size);
  ssize_t bytes_read;
  while (1) {
    // content_max_len - 1 to accomodate a '\0'
    if ((bytes_read = read(fd, content, content_max_size - 1)) == -1) {
      printErr("read failed: errno: %d", errno);
      free(content);
      return -1;
    }
    // -1 to overwrite the trailing new line
    content[bytes_read - 1] = '\0';
    if (type == 1) {
      if (atoll(content) > atoll(lim)) {
        free(content);
        return 0;
      }
    } else {
      if (strcmp(content, lim) > 0) {
        free(content);
        return 0;
      }
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
      printErr("lseek failed: errno: %d", errno);
      free(content);
      return -1;
    }

    if (nanosleep(&req, NULL) == -1) {
      printErr("nanosleep interrupted by a signal handler or encountered an error: errno: %d", errno);
      free(content);
      return -1;
    }
  }
}

static void endlessWait() {
  while (1) {
    sleep(100);
  }
}

// ------------------------- Helper Functions - End -----------------------

// -------------------------- mem - begin -----------------------------------

/*
  Resource residue:
    Note: Any residue in 'setMemLimit', except the created pid directory, and
    'memListener' is removed here
*/
static void memListenerCleanup(void *arg) {

  MemListenerCleanupPayload *mlcp = (MemListenerCleanupPayload *) arg;
  if (close(mlcp -> mlp -> oomefd) == -1) {
    printErr("close failed: errno: %d", errno);
  }
  if (close(mlcp -> mlp -> mocfd) == -1) {
    printErr("close failed: errno: %d", errno);
  }
  free(mlcp -> mlp -> pid_dir);
  free(mlcp -> mlp);
  free(mlcp);
}

/*
  Listens for OOM killer notification

  Resource residue:
    Created here and removed by 'memListenerCleanup':
      1 MemListenerCleanupPayload malloc

    Note: Any residue in 'setMemLimit' (except the created pid directory) and here
    is removed in 'memListenerCleanup'
*/
static void *memListener(void *arg) {

  MemListenerPayload *mlp = (MemListenerPayload *)arg;
  MemListenerCleanupPayload *cpl = malloc(sizeof(MemListenerCleanupPayload));
  cpl -> mlp = mlp;
  pthread_cleanup_push(memListenerCleanup, cpl);

  uint64_t u;
  ssize_t read_ret = read(mlp -> oomefd, &u, sizeof(uint64_t));

  int trylock_ret = pthread_mutex_trylock(mlp -> term_child_mutex);
  if (trylock_ret == 0) {
    // lock obtained
    if (read_ret == -1) {
      printErr("read failed: errno: %d", errno);
      *(mlp -> exceeded) = FATAL_ERROR_EXCEED;
    } else if (read_ret != sizeof(uint64_t)) {
      printErr("Unexpected return value from read: errno: %d", errno);
      *(mlp -> exceeded) = FATAL_ERROR_EXCEED;
    } else {
      *(mlp -> exceeded) = MEM_LIM_EXCEED;
    }

    printDebug("terminate called - memListener");
    if (terminatePid(mlp -> cg_locs, mlp -> pid) == -1) {
      printErr("terminatePid failed");
    }
    printDebug("terminate returned - memListener");
  } else if(trylock_ret == EBUSY) {
    // some other thread has the lock
  } else {
    // error
    printErr("pthread_mutex_trylock failed: errno: %d", errno);
  }
  endlessWait();
  pthread_cleanup_pop(1);
  return NULL;
}

/*
  Creates a memory pid directory, writes values to various files in this
  memory cgroup and then creates a thread that starts listening for OOM
  killer notification.

  Returns:
    -1 on any error
    0 on success

  Resource residue (when return value == 0):
    1 directory created - |pid_dir|
    1 char malloc - 'pid_dir'
    2 open fd's - 'oomefd', 'mocfd'
    1 MemListenerPayload malloc - 'mlp'
    1 thread created
*/
static int setMemLimit(
  pid_t pid, const char *mem, const CgroupLocs *cg_locs, int *exceeded,
  pthread_mutex_t *term_child_mutex, pthread_t *thread) {

  const char *memory_cg = cg_locs -> memory;
  char *pid_dir;
  if ((pid_dir = createPidDir(memory_cg, pid)) == NULL) {
    printErr("createPidDir failed");
    free(pid_dir);
    return -1;
  }
  // disable swap for the sandboxed executable because mem limits on swap
  // might not be available on all kernels
  if (writeToFile(pid_dir, "memory.swappiness", "0") == -1) {
    printErr("writeToFile failed");
    if (rmdir(pid_dir) == -1) {
      printErr("rmdir failed: errno: %d", errno);
    }
    free(pid_dir);
    return -1;
  }
  // disable oom killer, hence the process will be paused when under oom
  if (writeToFile(pid_dir, "memory.oom_control", "1") == -1) {
    printErr("writeToFile failed");
    if (rmdir(pid_dir) == -1) {
      printErr("rmdir failed: errno: %d", errno);
    }
    free(pid_dir);
    return -1;
  }
  // limit is rounded to greatest multiple of page size smaller than
  // |mem|, automatically
  if (writeToFile(pid_dir, "memory.limit_in_bytes", mem) == -1) {
    printErr("writeToFile failed");
    if (rmdir(pid_dir) == -1) {
      printErr("rmdir failed: errno: %d", errno);
    }
    free(pid_dir);
    return -1;
  }

  int oomefd = eventfd(0, 0);
  char *moc_path = malloc(sizeof(char) * (strlen(pid_dir) + 20));
  sprintf(moc_path, "%s/memory.oom_control", pid_dir);
  int mocfd = open(moc_path, O_RDONLY);
  char *buf = malloc((int)ceil(log10(oomefd)) + (int)ceil(log10(mocfd)) + 2);
  sprintf(buf, "%d %d", oomefd, mocfd);
  free(moc_path);
  if (writeToFile(pid_dir, "cgroup.event_control", buf) == -1) {
    free(buf);
    printErr("writeToFile failed");
    if (close(oomefd) == -1) {
      printErr("close failed: errno: %d", errno);
    }
    if (close(mocfd) == -1) {
      printErr("close failed: errno: %d", errno);
    }
    if (rmdir(pid_dir) == -1) {
      printErr("rmdir failed: errno: %d", errno);
    }
    free(pid_dir);
    return -1;
  }
  free(buf);

  MemListenerPayload *mlp = malloc(sizeof(MemListenerPayload));
  mlp -> oomefd = oomefd;
  mlp -> mocfd = mocfd;
  mlp -> exceeded = exceeded;
  mlp -> pid_dir = pid_dir;
  mlp -> pid = pid;
  mlp -> cg_locs = cg_locs;
  mlp -> term_child_mutex = term_child_mutex;
  pthread_create(thread, NULL, memListener, mlp);
  return 0;
}

// ---------------- mem - end -----------------------------------------------

// -----------------cpu_time - begin ----------------------------------------

/*
  Resource residue:
    Note: Any residue in 'setCpuTimeLimit' (except the created pid directory)
    and 'cpuTimeLimiter' is removed here.
*/
static void cpuTimeLimiterCleanup(void *arg) {

  CpuTimeLimiterCleanupPayload *ctlcp = (CpuTimeLimiterCleanupPayload *)arg;
  if (ctlcp -> fd != -1) {
    if (close(ctlcp -> fd) == -1) {
      printErr("close failed: errno: %d", errno);
    }
  }
  free(ctlcp -> ctlp -> pid_dir);
  free(ctlcp -> ctlp);
  free(ctlcp);
}

/*
  Polls the 'cpuacct.usage' file and uses 'terminate' if cpu time exceeds the
  limit.

  Resource residue:
    Created here and removed by 'cpuTimeLimiterCleanup':
      1 fd - 'fd'
      1 CpuTimeLimiterCleanupPayload malloc - 'ctlcp'
    Note: Any residue in 'setCpuTimeLimit' (except the created pid directory) and here
    is removed in 'cpuTimeLimiterCleanup'
*/
static void *cpuTimeLimiter(void *arg) {

  CpuTimeLimiterPayload *ctlp = (CpuTimeLimiterPayload *) arg;

  char *cpuacct_usage_path = malloc(
    sizeof(char) * (strlen(ctlp -> pid_dir) + 15));
  sprintf(cpuacct_usage_path, "%s/cpuacct.usage", ctlp -> pid_dir);
  int fd = open(cpuacct_usage_path, O_RDONLY);
  free(cpuacct_usage_path);

  // Set up cleanup handler
  CpuTimeLimiterCleanupPayload *ctlcp = malloc(
    sizeof(CpuTimeLimiterCleanupPayload));
  ctlcp -> fd = fd;
  ctlcp -> ctlp = ctlp;
  pthread_cleanup_push(cpuTimeLimiterCleanup, ctlcp);

  if (fd == -1) {

    int trylock_ret = pthread_mutex_trylock(ctlp -> term_child_mutex);
    if (trylock_ret == 0) {

      *(ctlp -> exceeded) = FATAL_ERROR_EXCEED;

      printDebug("terminate called - cpuTimeLimiter 1");
      if (terminatePid(ctlp -> cg_locs, ctlp -> pid)== -1) {
        printErr("terminatePid failed");
      }
      printDebug("terminate returned - cpuTimeLimiter 1");
    } else if (trylock_ret == EBUSY) {
      // some other thread has the lock
    } else {
      // error
      printErr("pthread_mutex_trylock failed: errno: %d", errno);
    }
    endlessWait();
  }

  // Blocks until time limit exceeds
  int poll_ret = pollFileContent(fd, ctlp -> cpu_time, TIME_LIM_POLL_INTERVAL, 1);
  int trylock_ret = pthread_mutex_trylock(ctlp -> term_child_mutex);
  if (trylock_ret == 0) {
    if (poll_ret == -1) {
      *(ctlp -> exceeded) = FATAL_ERROR_EXCEED;
    } else {
      *(ctlp -> exceeded) = TIME_LIM_EXCEED;
    }
    printDebug("terminate called - cpuTimeLimiter 2");
    if (terminatePid(ctlp -> cg_locs, ctlp -> pid) == -1) {
      printErr("terminatePid failed");
    }
    printDebug("terminate returned - cpuTimeLimiter 2");
  } else if (trylock_ret == EBUSY) {
    // some other thread has the lock
  } else {
    // error
    printErr("pthread_mutex_trylock failed: errno: %d", errno);
  }
  endlessWait();
  pthread_cleanup_pop(1);
  return NULL;
}

/*
  Creates a new pid directory and creates a new thread that enforces the time
  limit.

  Returns:
    -1 on error
    0 on success

  Resource residue (when return value == 0):
    1 char malloc - 'pid_dir'
    1 directory created - |pid_dir|
    1 CpuTimeLimiterPayload malloc - 'ctlp'
    1 thread
*/
static int setCpuTimeLimit(
  pid_t pid, const char *cpu_time, const CgroupLocs *cg_locs, int *exceeded,
  pthread_mutex_t *term_child_mutex, pthread_t *thread) {

  const char *cpuacct_cg = cg_locs -> cpuacct;
  char *pid_dir;
  if ((pid_dir = createPidDir(cpuacct_cg, pid)) == NULL) {
    printErr("createPidDir failed");
    free(pid_dir);
    return -1;
  }

  CpuTimeLimiterPayload *ctlp = malloc(sizeof(CpuTimeLimiterPayload));
  ctlp -> exceeded = exceeded;
  ctlp -> cpu_time = cpu_time;
  ctlp -> pid_dir = pid_dir;
  ctlp -> pid = pid;
  ctlp -> cg_locs = cg_locs;
  ctlp -> term_child_mutex = term_child_mutex;
  pthread_create(thread, NULL, cpuTimeLimiter, ctlp);
  return 0;
}

// -----------------cpu_time - end ------------------------------------------

// -----------------num_tasks - begin ---------------------------------------

/*
  Note: Any residue in 'setNumTasksLimit' (except the created pid
  directory) and in 'numTasksListener' is removed here
*/
static void numTasksListenerCleanup(void *arg) {

  NumTasksListenerCleanupPayload *ntlcp = (NumTasksListenerCleanupPayload *)arg;
  if (ntlcp -> fd != -1) {
    if (close(ntlcp -> fd) == -1) {
      printErr("close failed: errno: %d", errno);
    }
  }
  free(ntlcp -> ntlp -> pid_dir);
  free(ntlcp -> ntlp);
  free(ntlcp);
}

/*
  Polls 'pids.events' file to know if there was an attempt by the sandboxed
  executable to exceed the limit. This could be detected if the file content
  has a non zero value beside 'max', for instance 'max 1'

  Resource residue:
    Created here but removed by 'numTasksListenerCleanup':
      1 fd - 'fd'
      1 NumTasksListenerCleanupPayload malloc - 'ntlcp'
    Note: Any residue in 'setNumTasksLimit' (except the created pid
    directory) and here is removed in 'numTasksListenerCleanup'
*/
static void *numTasksListener(void *arg) {

  NumTasksListenerPayload *ntlp = (NumTasksListenerPayload *) arg;
  char *pids_events_path = malloc(
    sizeof(char) * (strlen(ntlp -> pid_dir) + 13));
  sprintf(pids_events_path, "%s/pids.events", ntlp -> pid_dir);
  int fd = open(pids_events_path, O_RDONLY);
  free(pids_events_path);

  // Setup the cleanup handler
  NumTasksListenerCleanupPayload *ntlcp = malloc(
    sizeof(NumTasksListenerCleanupPayload));
  ntlcp -> fd = fd;
  ntlcp -> ntlp = ntlp;
  pthread_cleanup_push(numTasksListenerCleanup, ntlcp);

  if (fd == -1) {
    printErr("open failed: errno: %d", errno);
    if (errno == ENOENT) {
      // Kernel version doesn't support 'pids.events'. The sandboxed
      // executable will not be terminated upon exceeding the number of pids
      // limit, but any calls to fork()/clone() after hitting the limit will
      // fail.
      endlessWait();
    }

    int trylock_ret = pthread_mutex_trylock(ntlp -> term_child_mutex);
    if (trylock_ret == 0) {
      *(ntlp -> exceeded) = FATAL_ERROR_EXCEED;
      printDebug("terminate called - numTasksListener 1");
      if (terminatePid(ntlp -> cg_locs, ntlp -> pid) == -1) {
        printErr("terminatePid failed");
      }
      printDebug("terminate returned - numTasksListener 1");
    } else if (trylock_ret == EBUSY) {
      // some other thread has the lock
    } else {
      // error
      printErr("pthread_mutex_trylock failed: errno: %d", errno);
    }
    endlessWait();
  }

  // Blocks until num tasks exceeds according to pids.events file
  int poll_ret = pollFileContent(fd, "max 0", NUM_TASKS_LIM_POLL_INTERVAL, 2);
  int trylock_ret = pthread_mutex_trylock(ntlp -> term_child_mutex);
  if (trylock_ret == 0) {
    if (poll_ret == -1) {
      *(ntlp -> exceeded) = FATAL_ERROR_EXCEED;
    } else {
      *(ntlp -> exceeded) = TASK_LIM_EXCEED;
    }

    printDebug("terminate called - numTasksListener 2");
    if (terminatePid(ntlp -> cg_locs, ntlp -> pid) == -1) {
      printErr("terminatePid failed");
    }
    printDebug("terminate returned - numTasksListener 2");
  } else if (trylock_ret == EBUSY) {
    // some other thread has the lock
  } else {
    // error
    printErr("pthread_mutex_trylock failed: errno: %d", errno);
  }

  endlessWait();
  pthread_cleanup_pop(1);
  return NULL;
}

/*
  Creates a new pid directory, writes the limit to a file and creates a
  thread that notifies when the limit exceeds.

  Returns:
    -1 on error
    0 on success

  Resource residue (if returned value == 0):
    1 char malloc - 'pid_dir'
    1 created directory - |pid_dir|
    1 NumTasksListenerPayload malloc - 'ntlp'
    1 thread
*/
static int setNumTasksLimit(
  pid_t pid, const char *num_tasks, const CgroupLocs *cg_locs, int *exceeded,
  pthread_mutex_t *term_child_mutex, pthread_t *thread) {

  const char *pids_cg = cg_locs -> pids;
  char *pid_dir;
  if ((pid_dir = createPidDir(pids_cg, pid)) == NULL) {
    printErr("createPidDir failed");
    free(pid_dir);
    return -1;
  }

  // Set the limit
  if (writeToFile(pid_dir, "pids.max", num_tasks) == -1) {
    printErr("writeToFile failed");
    if (rmdir(pid_dir) == -1) {
      printErr("rmdir failed: errno: %d", errno);
    }
    return -1;
  }

  // When a fork would cause number of pids allotted to exceed the pids
  // controller will cause the call to fail but does not terminate
  // any process. The 'numTasksListener' serves to notify when a process
  // tries to exceed its allotted limit and also use 'terminate' which
  // terminates the sanboxed executable and also the resource limiter
  // threads.
  // The takeaway is that enforcement of the limit is performed by the
  // controller itself like the case of enforcing memory limit and unlike
  // the case of enforcing CPU time limit.
  NumTasksListenerPayload *ntlp = malloc(sizeof(NumTasksListenerPayload));
  ntlp -> pid_dir = pid_dir;
  ntlp -> exceeded = exceeded;
  ntlp -> pid = pid;
  ntlp -> cg_locs = cg_locs;
  ntlp -> term_child_mutex = term_child_mutex;
  pthread_create(thread, NULL, numTasksListener, ntlp);
  return 0;
}

// -----------------num_tasks - end ------------------------------------------

/*
  Returns:
    0 on successfully setting limits
    -1 on failure

  Errors ocurring during running of the threads are registered by setting
  '*exceeded' appropriately as these might occur after 'set*Limit'
  function returns thus reflecting this error in the return value of
  'set*Limit' would not be possible.

  '*exceeded' == FATAL_ERROR_EXCEED when an error causes the sandbox to be non
  functional. Now, the sanboxed executable is killed, all resource limiter
  threads are terminated.
*/
int setResourceLimits(
  pid_t pid, const ResLimits *res_limits, const CgroupLocs *cg_locs,
  int *exceeded, pthread_mutex_t *term_child_mutex,
  pthread_t *watcher_threads, int num_watcher_threads) {

  if (setMemLimit(
    pid, res_limits -> mem, cg_locs, exceeded,
    term_child_mutex, &watcher_threads[0]) == -1) {
    printErr("setMemLimit failed");
    return -1;
  }

  if (setCpuTimeLimit(
    pid, res_limits -> cpu_time, cg_locs, exceeded,
    term_child_mutex, &watcher_threads[1]) == -1) {
    printErr("setCpuTimeLimit failed");
    return -1;
  }

  if (setNumTasksLimit(
    pid, res_limits -> num_tasks, cg_locs, exceeded,
    term_child_mutex, &watcher_threads[2]) == -1) {
    printErr("setNumTasksLimit failed");
    return -1;
  }

  if (addToTasksFile(cg_locs -> memory, pid) == -1) {
    printErr("addToTasksFile failed");
    return -1;
  }
  if (addToTasksFile(cg_locs -> cpuacct, pid) == -1) {
    printErr("addToTasksFile failed");
    return -1;
  }
  if (addToTasksFile(cg_locs -> pids, pid) == -1) {
    printErr("addToTasksFile failed");
    return -1;
  }
  return 0;
}