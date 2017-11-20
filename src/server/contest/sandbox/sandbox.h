#ifndef SANDBOX_H_
#define SANDBOX_H_

#include <sys/types.h>

// So that including 'sandbox.h' in user application is enough for using
// 'CgroupLocs' and 'ResLimits'
#include "resource_limits.h"

// Serve as return values, hence kept non-negative
#define SB_FAILURE 1
#define SB_RUNTIME_ERR 2
#define SB_OK 0
#define SB_MEM_EXCEED 3
#define SB_TIME_EXCEED 4
#define SB_TASK_EXCEED 5

int sandboxExec(
  const char *exect_path, const char *jail_path,
  const char *input_file, const char *output_file,
  const CgroupLocs *cg_locs, const ResLimits *res_lims,
  const char *whitelist, uid_t uid, gid_t gid);

#endif