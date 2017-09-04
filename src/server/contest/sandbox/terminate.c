#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

#include "terminate.h"
#include "resource_limits.h"
#include "logger.h"

static char *getPidDir(const char *cg, pid_t pid) {

  char *pid_dir = malloc(
    sizeof(char) * (strlen(cg) + (int)ceil(log10(pid)) + 2));
  sprintf(pid_dir, "%s/%d", cg, pid);
  return pid_dir;
}

int removePidDirs(const CgroupLocs *cg_locs, pid_t pid) {
  char *memory_cg = getPidDir(cg_locs -> memory, pid);
  char *cpuacct_cg = getPidDir(cg_locs -> cpuacct, pid);
  char *pids_cg = getPidDir(cg_locs -> pids, pid);

  int ret = 0;
  if (rmdir(memory_cg) == -1) {
    printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
    ret = -1;
  }
  if (rmdir(cpuacct_cg) == -1) {
    printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
    ret = -1;
  }
  if (rmdir(pids_cg) == -1) {
    printErr(__FILE__, __LINE__, "rmdir failed", 1, errno);
    ret = -1;
  }

  free(memory_cg);
  free(cpuacct_cg);
  free(pids_cg);
  return ret;
}

int terminate(TerminatePayload *tp) {
  // Just for safety so that the function doesn't get called twice
  if (tp -> once == 1) {
    return 0;
  }
  tp -> once = 1;
  int ret = 0;
  if (!(tp -> terminated)) {
    // TODO: handle corner case of killing init in PID NS
    if (kill(tp -> pid, SIGKILL) == -1) {
      printErr(__FILE__, __LINE__, "kill failed", 1, errno);
      ret = -1;
    }

    // wait until 'sandboxExec' confirms the process has terminated
    while (tp -> terminated == 0);

    #ifdef SB_VERBOSE
    printf("Killed %d\n", tp -> pid);
    #endif
  }

  int i;
  for (i = 0; i < (tp -> threads_len); i++) {
    if ((tp -> skip == NULL) || pthread_equal((tp -> threads)[i], *(tp -> skip)) == 0) {
      pthread_cancel((tp -> threads)[i]);
      #ifdef SB_VERBOSE
      printf("Killed thread: %d\n", i);
      #endif
      pthread_join((tp -> threads)[i], NULL);
    }
  }

  if (removePidDirs(tp -> cg_locs, tp -> pid) == -1) {
    printErr(__FILE__, __LINE__, "removePidDirs failed", 1, errno);
    ret = -1;
  }
  tp -> done = 1;
  return 0;
}