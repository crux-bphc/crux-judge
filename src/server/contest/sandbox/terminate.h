#ifndef TERMINATE_H_
#define TERMINATE_H_

#include <pthread.h>
#include <sys/types.h>

#include "resource_limits.h"

typedef struct TerminatePayload {
  pthread_t *threads;
  // threads[i] that is equal to *skip is not cancelled; all others are
  pthread_t *skip;
  int threads_len;
  int terminated; // 1 if the sandboxed executable is terminated
  int done; // becomes 1 (from 0) when the 'terminate' completes once
  int once; // 1 if 'terminate' is called atleast once else 0
  pid_t pid;
  const CgroupLocs *cg_locs;
} TerminatePayload;

int removePidDirs(const CgroupLocs *cg_locs, pid_t pid);

int terminate(TerminatePayload *tp);

#endif