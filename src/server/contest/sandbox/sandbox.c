#define _GNU_SOURCE // for clone(); has to be before the #includes

#include <stdio.h>
#include <stdlib.h> // malloc()
#include <sched.h> // clone()
#include <errno.h> // errno
#include <unistd.h> // dup(), getpid(), STDIN_FILENO, STDOUT_FILENO, execl()
#include <fcntl.h> // open()
#include <signal.h> // SIGCHLD
#include <sys/wait.h> // waitpid()
#include <sys/types.h> // pid_t, open(), waitpid()
#include <sys/stat.h> // open()
#include <sys/eventfd.h> // eventfd()
#include <stdint.h>
#include <seccomp.h>
#include <semaphore.h>

#include "logger.h"
#include "syscall_manager.h"
#include "sandbox.h"
#include "resource_limits.h"
#include "terminate.h"

#define EXIT_CHILD_FAILURE 1

typedef struct ChildPayload {
  const char *exect_path;
  const char *jail_path;
  const char *input_file;
  const char *output_file;
  const char *whitelist;
  scmp_filter_ctx *ctx;
  uid_t uid;
  gid_t gid;
} ChildPayload;

static int childFunc(void *arg) {

  ChildPayload *cp = (ChildPayload *)arg;
  int in = open(cp -> input_file, O_RDONLY);
  if (in == -1) {
    printErr("open failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  int out = open(cp -> output_file, O_WRONLY | O_CREAT | O_TRUNC);
  if (out == -1) {
    printErr("open failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  // Redirect stdio of child process
  if (dup2(in, STDIN_FILENO) == -1) {
    printErr("dup2 failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (dup2(out, STDOUT_FILENO) == -1) {
    printErr("dup2 failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (close(in) == -1) {
    printErr("close failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (close(out) == -1) {
    printErr("close failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }

  // Notify parent to set resource limits and start accounting time, set
  // memory limits etc.
  // Notifies parent which then sets resource limits
  sem_t *notify_p = sem_open("notify_p", 0);
  if (sem_post(notify_p) == -1) {
    printErr("sem_post failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  // wait until resource limits are set in the parent and the parent
  // notifies
  sem_t *notify_c = sem_open("notify_c", 0);
  if (sem_wait(notify_c) == -1) {
    printErr("sem_wait failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (sem_destroy(notify_p) == -1) {
    printErr("sem_destroy failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (sem_destroy(notify_c) == -1) {
    printErr("sem_destroy failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }

  // This is required because call to 'installSysCallBlocker' is made after
  // 'chroot' and the former requires to access contents of 'whitelist'
  // O_CLOEXEC is not really necessary since we're trying to close in
  // anyway in 'installSysCallBlocker'
  int whitelist_fd = open(cp -> whitelist, O_RDONLY | O_CLOEXEC);
  if (whitelist_fd == -1) {
    printErr("open failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }

  // 'chdir', 'chroot' and drop privileges
  if (chdir(cp -> jail_path) == -1) {
    printErr("chdir failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (chroot("./") == -1) {
    printErr("chroot failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  // uid and gid persist even after exec and child processes inherit these
  // from the parent process. Hence all the processes of the executable will
  // have this uid and gid
  // First gid must be set and only then uid
  if (setgid(cp -> gid) == -1) {
    printErr("setgid failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }
  if (setuid(cp -> uid) == -1) {
    printErr("setuid failed: errno: %d", errno);
    return EXIT_CHILD_FAILURE;
  }

  // System calls not in whitelist follow action that was specified in the
  // call to 'seccomp_init'
  if (installSysCallBlocker(cp -> ctx, whitelist_fd) == -1) {
    printErr("installSysCallBlocker failed");
    return EXIT_CHILD_FAILURE;
  }

  if (execl(cp -> exect_path, cp -> exect_path, (char *)NULL) == -1) {
    printErr("execl failed: errno: %d", errno);
  }
  return EXIT_CHILD_FAILURE;
}

static int sandboxExecFailCleanup(
  sem_t *notify_p, sem_t *notify_c, char *child_stack,
  scmp_filter_ctx *ctx) {

  free(child_stack);
  free(ctx);
  int ret = 0;
  if (sem_destroy(notify_p) == -1) {
    printErr("sem_destroy failed: errno: %d", errno);
    ret = -1;
  }
  if (sem_destroy(notify_c) == -1) {
    printErr("sem_destroy failed: errno: %d", errno);
    ret = -1;
  }
  return ret;
}

/*
  Returns:
    SB_FAILURE
    SB_RUNTIME_ERR
    SB_OK
    SB_MEM_EXCEED
    SB_TIME_EXCEED
    SB_TASK_EXCEED
*/
int sandboxExec(
  const char *exect_path, const char *jail_path,
  const char *input_file, const char *output_file,
  const CgroupLocs *cg_locs, const ResLimits *res_lims,
  const char *whitelist, uid_t uid, gid_t gid) {

  sem_t *notify_p = sem_open("notify_p", O_CREAT, O_RDWR, 0);
  sem_t *notify_c = sem_open("notify_c", O_CREAT, O_RDWR, 0);
  scmp_filter_ctx *ctx = malloc(sizeof(scmp_filter_ctx));

  // ------------------ clone ------------------
  // TODO: what should child_stack_size be set to,
  // considering mem limits will be placed on child proc?
  long int child_stack_size = 1024 * 1024;
  char *child_stack = malloc(child_stack_size);
  if (child_stack == NULL) {
    printErr("malloc failed");
    return SB_FAILURE;
  }
  ChildPayload cp;
  cp.exect_path = exect_path;
  cp.jail_path = jail_path;
  cp.input_file = input_file;
  cp.output_file = output_file;
  cp.whitelist = whitelist;
  cp.ctx = ctx;
  cp.uid = uid;
  cp.gid = gid;

  // assuming downwardly growing stack
  // this pid is (also) the pid from kernel view
  pid_t pid = clone(
    childFunc, child_stack + child_stack_size, CLONE_NEWPID | SIGCHLD,
    &cp);
  if (pid == -1) {
    printErr("clone failed: errno: %d", errno);
    if (sandboxExecFailCleanup(notify_p, notify_c, child_stack, ctx) == -1) {
      printErr("sandboxExecFailCleanup failed: errno: %d", errno);
    }
    return SB_FAILURE;
  }

  // ------------------ set resource limits ------------------
  // wait until child notifies
  if (sem_wait(notify_p) == -1) {
    printErr("sem_wait failed: errno: %d", errno);
    if (kill(pid, SIGTERM) == -1) {
      printErr("kill failed: errno: %d", errno);
    }
    if (sandboxExecFailCleanup(notify_p, notify_c, child_stack, ctx) == -1) {
      printErr("sandboxExecFailCleanup failed: errno: %d", errno);
    }
    return SB_FAILURE;
  }

  int exceeded = NO_EXCEED;
  int num_watcher_threads = 3;
  pthread_t watcher_threads[num_watcher_threads];
  pthread_mutex_t term_child_mutex = PTHREAD_MUTEX_INITIALIZER;
  if (setResourceLimits(
    pid, res_lims, cg_locs, &exceeded, &term_child_mutex,
    watcher_threads, num_watcher_threads) == -1) {
    printErr("setResourceLimits failed");

    if (kill(pid, SIGTERM) == -1) {
      printErr("kill failed: errno: %d", errno);
    }

    if (sandboxExecFailCleanup(notify_p, notify_c, child_stack, ctx) == -1) {
      printErr("sandboxExecFailCleanup failed: errno: %d", errno);
    }
    return SB_FAILURE;
  }

  // notify child that resource limits are set
  if (sem_post(notify_c) == -1) {
    printErr("sem_post failed: errno: %d", errno);

    if (kill(pid, SIGTERM) == -1) {
      printErr("kill failed: errno: %d", errno);
    }

    if (removePidDirs(cg_locs, pid) == -1) {
      printErr("removePidDirs failed: errno: %d", errno);
    }

    if (sandboxExecFailCleanup(notify_p, notify_c, child_stack, ctx) == -1) {
      printErr("sandboxExecFailCleanup failed: errno: %d", errno);
    }
    return SB_FAILURE;
  }

  // ------------------ wait for child to terminate ------------------
  int wstatus;
  waitpid(pid, &wstatus, 0);

  // Used to handle the case where a limit is found to be exceeded, but
  // before the sandboxed process is terminated, it itself exits. This
  // could result in the corresponding watcher thread, trying to terminate
  // an already terminated process.
  // The following code tries to lock the mutex so that, the watcher thread
  // cannot terminate the already terminated process. However, this is not
  // perfect because the watcher thread could lock the mutex before the
  // following code does.
  int trylock_ret = pthread_mutex_trylock(&term_child_mutex);
  if (trylock_ret == 0) {
    // lock obtained
  } else if(trylock_ret == EBUSY) {
    // some other thread has the lock
  } else {
    // error
    printErr("pthread_mutex_trylock failed: errno: %d", errno);
  }

  free(child_stack);
  if (*ctx != NULL) {
    seccomp_release(*ctx);
  }
  free(ctx);

  // cleanup resources used by setResource limit
  terminateThreads(watcher_threads, num_watcher_threads);
  if (removePidDirs(cg_locs, pid) == -1) {
    printErr("removePidDirs failed: errno: %d", errno);
  }

  printResult("Results");
  printResult("```````");
  if (WIFEXITED(wstatus)) {
    printResult("Child exited with exit status: %d", WEXITSTATUS(wstatus));
    if (WEXITSTATUS(wstatus) == EXIT_CHILD_FAILURE) {
      printErr("Sandbox failure");
      return SB_FAILURE;
    }
  } else if (WIFSIGNALED(wstatus)) {
    printResult("Child terminated with signal: %d", WTERMSIG(wstatus));
  } else {
    printErr("Unexpected: Child neither exited nor signaled");
    return SB_FAILURE;
  }

  switch(exceeded) {
    case NO_EXCEED:
      if (WIFSIGNALED(wstatus)) {
        printResult("Runtime error");
        return SB_RUNTIME_ERR;
      } else {
        printResult("All OK");
        return SB_OK;
      }
    case FATAL_ERROR_EXCEED:
      printErr("Sandbox failure");
      return SB_FAILURE;
    case MEM_LIM_EXCEED:
      printResult("Memory limit exceeded");
      return SB_MEM_EXCEED;
    case TIME_LIM_EXCEED:
      printResult("Time limit exceeded");
      return SB_TIME_EXCEED;
    case TASK_LIM_EXCEED:
      printResult("Task limit exceeded");
      return SB_TASK_EXCEED;
    default:
      printErr("Unexpected value for exceeded");
      return SB_FAILURE;
  }
}