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
#include <fcntl.h>

#include "terminate.h"
#include "resource_limits.h"
#include "logger.h"

#define MAX_STAT_READ_SIZE 50 // in bytes

// -------------------- Helper Functions - Begin ---------------------------
/*
  Returns a pointer to an NTCS |cg|/|pid|.

  Resource residue:
    1 char malloc - 'pid_dir'
*/
char *getPidDir(const char *cg, pid_t pid) {
  char *pid_dir = malloc(
    sizeof(char) * (strlen(cg) + (int)ceil(log10(pid)) + 2));
  sprintf(pid_dir, "%s/%d", cg, pid);
  return pid_dir;
}

int removePidDirs(const CgroupLocs *cg_locs, pid_t pid) {
  char *memory_pid_dir = getPidDir(cg_locs -> memory, pid);
  char *cpuacct_pid_dir = getPidDir(cg_locs -> cpuacct, pid);
  char *pids_pid_dir = getPidDir(cg_locs -> pids, pid);

  int ret = 0;
  if (rmdir(memory_pid_dir) == -1) {
    printErr("rmdir failed: errno: %d", errno);
    ret = -1;
  }
  if (rmdir(cpuacct_pid_dir) == -1) {
    printErr("rmdir failed: errno: %d", errno);
    ret = -1;
  }
  if (rmdir(pids_pid_dir) == -1) {
    printErr("rmdir failed: errno: %d", errno);
    ret = -1;
  }

  free(memory_pid_dir);
  free(cpuacct_pid_dir);
  free(pids_pid_dir);
  return ret;
}

static char *readFile(const char *dir, const char *file) {

  char *path = malloc(sizeof(char) * (strlen(dir) + strlen(file) + 2));
  sprintf(path, "%s/%s", dir, file);

  int fd = open(path, O_RDONLY);
  free(path);
  if (fd == -1) {
    printErr("open failed: errno: %d", errno);
    return NULL;
  }

  char *buf = malloc(MAX_STAT_READ_SIZE);
  ssize_t bytes_read;
  if ((bytes_read = read(fd, buf, MAX_STAT_READ_SIZE)) == -1) {
    printErr("read failed: errno: %d", errno);
    if (close(fd) == -1) {
      printErr("close failed: errno: %d", errno);
    }
    if (buf != NULL) {
      free(buf);
    }
    return NULL;
  }

  // -1 to overwrite the trailing '\n'
  buf[bytes_read - 1] = '\0';
  if (close(fd) == -1) {
    printErr("close failed: errno: %d", errno);
    free(buf);
    return NULL;
  }
  return buf;
}

static int printStats(const CgroupLocs *cg_locs, pid_t pid) {

  char *buf;
  char *memory_pid_dir = getPidDir(cg_locs -> memory, pid);
  if ((buf = readFile(memory_pid_dir, "memory.max_usage_in_bytes")) == NULL) {
    printErr("readFile failed: errno: %d", errno);
    free(memory_pid_dir);
    return -1;
  }
  printf("Stats\n");
  printf("`````\n");
  printf("Memory usage in bytes: %s\n", buf);
  free(buf);
  free(memory_pid_dir);

  char *cpuacct_pid_dir = getPidDir(cg_locs -> cpuacct, pid);
  if ((buf = readFile(cpuacct_pid_dir, "cpuacct.usage")) == NULL) {
    printErr("readFile failed: errno: %d", errno);
    free(cpuacct_pid_dir);
    return -1;
  }
  printf("CPU time in nanoseconds: %s\n", buf);
  free(buf);
  free(cpuacct_pid_dir);

  char *pids_pid_dir= getPidDir(cg_locs -> pids, pid);
  if ((buf = readFile(pids_pid_dir, "pids.current")) == NULL) {
    printErr("readFile failed: errno: %d", errno);
    free(pids_pid_dir);
    return -1;
  }
  printf("Current number of tasks: %s\n\n", buf);
  free(buf);
  free(pids_pid_dir);
  return 0;
}

// -------------------- Helper Functions - End ----------------------------

int terminatePid(const CgroupLocs *cg_locs, pid_t pid) {
  int ret = 0;

  #ifdef SB_PRINT_STATS
  if (printStats(cg_locs, pid) == -1) {
    printErr("printStats failed");
    ret = -1;
  }
  #endif

  if (kill(pid, SIGKILL) == -1) {
    printErr("kill failed: errno: %d", errno);
    ret = -1;
  }
  return ret;
}

void terminateThreads(pthread_t *threads, int num_threads) {
  int i;
  for (i = 0; i < num_threads; i++) {
    if (pthread_cancel(threads[i]) == -1) {
      printErr("pthread_cancel failed: errno: %d", errno);
    }
  }
  for (i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }
}