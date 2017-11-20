#ifndef TERMINATE_H_
#define TERMINATE_H_

#include <pthread.h>
#include <sys/types.h>

#include "resource_limits.h"

char *getPidDir(const char *cg, pid_t pid);

int removePidDirs(const CgroupLocs *cg_locs, pid_t pid);

int terminatePid(const CgroupLocs *cg_locs, pid_t pid);

void terminateThreads(pthread_t *threads, int num_threads);

#endif