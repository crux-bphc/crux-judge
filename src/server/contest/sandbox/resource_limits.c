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

#include "logger.h"
#include "resource_limits.h"
#include "terminate.h"

#define TIME_LIM_POLL_INTERVAL 100 // nanoseconds
#define NUM_TASKS_LIM_POLL_INTERVAL 10 // nanoseconds

typedef struct MemListenerPayload {
  int oomefd;
  int mocfd;
  int *exceeded;
  char *pid_dir;
  TerminatePayload *tp;
} MemListenerPayload;

typedef struct MemListenerCleanupPayload {
  MemListenerPayload *mlp;
} MemListenerCleanupPayload;

typedef struct CpuTimeLimiterPayload {
  int *exceeded;
  const char *cpu_time;
  char *pid_dir;
  TerminatePayload *tp;
} CpuTimeLimiterPayload;

typedef struct CpuTimeLimiterCleanupPayload {
  int fd;
  CpuTimeLimiterPayload *ctlp;
} CpuTimeLimiterCleanupPayload;

typedef struct NumTasksListenerPayload {
  int *exceeded;
  char *pid_dir;
  TerminatePayload *tp;
} NumTasksListenerPayload;

typedef struct NumTasksListenerCleanupPayload {
  int fd;
  char *pids_events_path;
  NumTasksListenerPayload *ntlp;
} NumTasksListenerCleanupPayload;

// ------------------------- Helper Functions - Begin -----------------------

/*
  Returns a pointer to an NTCS |cg|/|pid|.

  Resource residue:
    1 char malloc - 'pid_dir'
*/
static char *getPidDir(const char *cg, pid_t pid) {
  char *pid_dir = malloc(
    sizeof(char) * (strlen(cg) + (int)ceil(log10(pid)) + 2));
  sprintf(pid_dir, "%s/%d", cg, pid);
  return pid_dir;
}

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
    printErr(__FILE__, __LINE__, "mkdir failed", 1, errno);
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
    printErr(__FILE__, __LINE__, "open failed", 1, errno);
    free(path);
    return -1;
  }
  if (write(f, str, sizeof(char) * strlen(str)) == -1) {
    printErr(__FILE__, __LINE__, "write failed", 1, errno);
    free(path);
    return -1;
  }
  if (close(f) == -1) {
    printErr(__FILE__, __LINE__, "close failed", 1, errno);
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
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
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
      printErr(__FILE__, __LINE__, "read failed", 1, errno);
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
      printErr(__FILE__, __LINE__, "lseek failed", 1, errno);
      free(content);
      return -1;
    }

    if (nanosleep(&req, NULL) == -1) {
      printErr(__FILE__, __LINE__, "nanosleep interrupted by a signal \
        handler or encountered an error", 1, errno);
      free(content);
      return -1;
    }
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
    printErr(__FILE__, __LINE__, "close failed", 1, errno);
  }
  if (close(mlcp -> mlp -> mocfd) == -1) {
    printErr(__FILE__, __LINE__, "close failed", 1, errno);
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
  ssize_t ret = read(mlp -> oomefd, &u, sizeof(uint64_t));
  if (ret == -1) {
    printErr(__FILE__, __LINE__, "read failed", 1, errno);
    *(mlp -> exceeded) = FATAL_ERROR_EXCEED;
  } else if (ret != sizeof(uint64_t)) {
    printErr(__FILE__, __LINE__, "Unexpected return value from read", 1, errno);
    *(mlp -> exceeded) = FATAL_ERROR_EXCEED;
  } else {
    *(mlp -> exceeded) = MEM_LIM_EXCEED;
  }
  mlp -> tp -> skip = malloc(sizeof(pthread_t));
  *(mlp -> tp -> skip) = pthread_self();
  #ifdef SB_VERBOSE
  printf("terminate called - memListener\n");
  #endif
  if (terminate(mlp -> tp) == -1) {
    printErr(__FILE__, __LINE__, "terminate failed", 0, 0);
  }
  #ifdef SB_VERBOSE
  printf("terminate returned - memListener\n");
  #endif

  pthread_cleanup_pop(0);
  pthread_exit(NULL);
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
  pid_t pid, const char *mem, const char *memory_cg, int *exceeded,
  pthread_t *thread, TerminatePayload *tp) {

  char *pid_dir;
  if ((pid_dir = createPidDir(memory_cg, pid)) == NULL) {
    printErr(__FILE__, __LINE__, "createPidDir failed", 0, 0);
    free(pid_dir);
    return -1;
  }
  // disable swap for the sandboxed executable because mem limits on swap
  // might not be available on all kernels
  if (writeToFile(pid_dir, "memory.swappiness", "0") == -1) {
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
    if (rmdir(pid_dir) == -1) {
      printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
    }
    free(pid_dir);
    return -1;
  }
  // disable oom killer, hence the process will be paused when under oom
  if (writeToFile(pid_dir, "memory.oom_control", "1") == -1) {
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
    if (rmdir(pid_dir) == -1) {
      printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
    }
    free(pid_dir);
    return -1;
  }
  // limit is rounded to greatest multiple of page size smaller than
  // |mem|, automatically
  if (writeToFile(pid_dir, "memory.limit_in_bytes", mem) == -1) {
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
    if (rmdir(pid_dir) == -1) {
      printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
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
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
    if (close(oomefd) == -1) {
      printErr(__FILE__, __LINE__, "close failed", 1, errno);
    }
    if (close(mocfd) == -1) {
      printErr(__FILE__, __LINE__, "close failed", 1, errno);
    }
    if (rmdir(pid_dir) == -1) {
      printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
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
  mlp -> tp = tp;
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
      printErr(__FILE__, __LINE__, "close failed", 1, errno);
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
    printErr(__FILE__, __LINE__, "open failed", 1, errno);
    *(ctlp -> exceeded) = FATAL_ERROR_EXCEED;

    ctlp -> tp -> skip = malloc(sizeof(pthread_t));
    *(ctlp -> tp -> skip) = pthread_self();
    #ifdef SB_VERBOSE
    printf("terminate called - cpuTimeLimiter 1\n");
    #endif
    if (terminate(ctlp -> tp)== -1) {
      printErr(__FILE__, __LINE__, "terminate failed", 0, 0);
    }
    #ifdef SB_VERBOSE
    printf("terminate returned - cpuTimeLimiter 1\n");
    #endif

    pthread_exit(NULL);
  }

  // Blocks until time limit exceeds
  if (pollFileContent(fd, ctlp -> cpu_time, TIME_LIM_POLL_INTERVAL, 1) == -1) {
    *(ctlp -> exceeded) = FATAL_ERROR_EXCEED;
  } else {
    *(ctlp -> exceeded) = TIME_LIM_EXCEED;
  }

  ctlp -> tp -> skip = malloc(sizeof(pthread_t));
  *(ctlp -> tp -> skip) = pthread_self();
  #ifdef SB_VERBOSE
  printf("terminate called - cpuTimeLimiter 2\n");
  #endif
  if (terminate(ctlp -> tp) == -1) {
    printErr(__FILE__, __LINE__, "terminate failed", 0, 0);
  }
  #ifdef SB_VERBOSE
  printf("terminate returned - cpuTimeLimiter 2\n");
  #endif

  pthread_cleanup_pop(0);
  pthread_exit(NULL);
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
  pid_t pid, const char *cpu_time, const char *cpuacct_cg, int *exceeded,
  pthread_t *thread, TerminatePayload *tp) {

  char *pid_dir;
  if ((pid_dir = createPidDir(cpuacct_cg, pid)) == NULL) {
    printErr(__FILE__, __LINE__, "createPidDir failed", 0, 0);
    free(pid_dir);
    return -1;
  }

  CpuTimeLimiterPayload *ctlp = malloc(sizeof(CpuTimeLimiterPayload));
  ctlp -> exceeded = exceeded;
  ctlp -> cpu_time = cpu_time;
  ctlp -> pid_dir = pid_dir;
  ctlp -> tp = tp;
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
      printErr(__FILE__, __LINE__, "close failed", 1, errno);
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
    printErr(__FILE__, __LINE__, "open failed", 1, errno);

    *(ntlp -> exceeded) = FATAL_ERROR_EXCEED;

    ntlp -> tp -> skip = malloc(sizeof(pthread_t));
    *(ntlp -> tp -> skip) = pthread_self();
    #ifdef SB_VERBOSE
    printf("terminate called - numTasksListener 1\n");
    #endif
    if (terminate(ntlp -> tp) == -1) {
      printErr(__FILE__, __LINE__, "terminate failed", 0, 0);
    }
    #ifdef SB_VERBOSE
    printf("terminate returned - numTasksListener 1\n");
    #endif

    pthread_exit(NULL);
  }

  // Blocks until num tasks exceeds according to pids.events file
  if (pollFileContent(fd, "max 0", NUM_TASKS_LIM_POLL_INTERVAL, 2) == -1) {
    *(ntlp -> exceeded) = FATAL_ERROR_EXCEED;
  } else {
    *(ntlp -> exceeded) = TASK_LIM_EXCEED;
  }

  ntlp -> tp -> skip = malloc(sizeof(pthread_t));
  *(ntlp -> tp -> skip) = pthread_self();
  #ifdef SB_VERBOSE
  printf("terminate called - numTasksListener 2\n");
  #endif
  if (terminate(ntlp -> tp) == -1) {
    printErr(__FILE__, __LINE__, "terminate failed", 0, 0);
  }
  #ifdef SB_VERBOSE
  printf("terminate returned - numTasksListener 2\n");
  #endif

  pthread_cleanup_pop(0);
  pthread_exit(NULL);
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
  pid_t pid, const char *num_tasks, const char *pids_cg, int *exceeded,
  pthread_t *thread, TerminatePayload *tp) {

  char *pid_dir;
  if ((pid_dir = createPidDir(pids_cg, pid)) == NULL) {
    printErr(__FILE__, __LINE__, "createPidDir failed", 0, 0);
    free(pid_dir);
    return -1;
  }

  // Set the limit
  if (writeToFile(pid_dir, "pids.max", num_tasks) == -1) {
    printErr(__FILE__, __LINE__, "writeToFile failed", 0, 0);
    if (rmdir(pid_dir) == -1) {
      printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
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
  ntlp -> tp = tp;
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
  int *exceeded, TerminatePayload **pl) {

  TerminatePayload *tp = malloc(sizeof(TerminatePayload));
  tp -> cg_locs = cg_locs;
  tp -> pid = pid;
  tp -> threads_len = 3;
  tp -> skip = NULL;
  tp -> terminated = 0;
  tp -> done = 0;
  tp -> once = 0;
  pthread_t *threads = malloc(sizeof(pthread_t) * (tp -> threads_len));
  tp -> threads = threads;
  *pl = tp;

  if (setMemLimit(
    pid, res_limits -> mem, cg_locs -> memory, exceeded,
    &threads[0], tp) == -1) {
    printErr(__FILE__, __LINE__, "setMemLimit failed", 0, 0);
    return -1;
  }

  if (setCpuTimeLimit(
    pid, res_limits -> cpu_time, cg_locs -> cpuacct, exceeded,
    &threads[1], tp) == -1) {
    printErr(__FILE__, __LINE__, "setCpuTimeLimit failed", 0, 0);
    return -1;
  }

  if (setNumTasksLimit(
    pid, res_limits -> num_tasks, cg_locs -> pids, exceeded,
    &threads[2], tp) == -1) {
    printErr(__FILE__, __LINE__, "setNumTasksLimit failed", 0, 0);
    return -1;
  }

  if (addToTasksFile(cg_locs -> memory, pid) == -1) {
    printErr(__FILE__, __LINE__, "addToTasksFile failed", 0, 0);
    return -1;
  }
  if (addToTasksFile(cg_locs -> cpuacct, pid) == -1) {
    printErr(__FILE__, __LINE__, "addToTasksFile failed", 0, 0);
    return -1;
  }
  if (addToTasksFile(cg_locs -> pids, pid) == -1) {
    printErr(__FILE__, __LINE__, "addToTasksFile failed", 0, 0);
    return -1;
  }
  return 0;
}