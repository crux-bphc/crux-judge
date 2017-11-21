#ifndef RESOURCE_LIMITS_H_
#define RESOURCE_LIMITS_H_

#include <sys/types.h>

// For 'exceeded'
#define FATAL_ERROR_EXCEED 4
#define NO_EXCEED 0
#define MEM_LIM_EXCEED 1
#define TIME_LIM_EXCEED 2
#define TASK_LIM_EXCEED 3

typedef struct ResLimits {
  const char *cpu_time; // nanoseconds
  const char *mem; // bytes
  const char *num_tasks; // max number of pids allotted
} ResLimits;

typedef struct CgroupLocs {
  // Location to a cgroup(directory) in a file system of type 'cgroup' with
  // mount option specifying a 'memory' controller. One directory per
  // sandboxed executable will be created in this cgroup.
  const char *memory;
  // Location to a cgroup(directory) in a file system of type 'cgroup' with
  // mount option specifying a 'cpuacct' controller. One directory per
  // sandboxed executable will be created in this cgroup.
  const char *cpuacct;
  // Location to a cgroup(directory) in a file system of type 'cgroup' with
  // mount option specifying a 'pids' controller. One directory per
  // sandboxed executable will be created in this cgroup.
  const char *pids;

  // Directory paths should not have trailing forward slash
} CgroupLocs;

int setResourceLimits(
  pid_t pid, const ResLimits *res_limits, const CgroupLocs *cg_locs,
  int *exceeded, pthread_mutex_t *terminate_child,
  pthread_t *watcher_threads, int num_watcher_threads);

#endif