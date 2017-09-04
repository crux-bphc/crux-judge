#ifndef SYSCALL_MANAGER_H
#define SYSCALL_MANAGER_H

#include <seccomp.h>

/*
  'ctx' is the filter context and 'whitelist' is the fd to an 'open(2)'ed
  file which contains system calls that should be allowed.
*/
int installSysCallBlocker(scmp_filter_ctx *ctx, int whitelist_fd);

#endif