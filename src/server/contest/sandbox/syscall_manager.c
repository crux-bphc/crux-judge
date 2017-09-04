#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <seccomp.h>
#include <errno.h>

#include "logger.h"

static int installSysCallBlockerCleanup(
  scmp_filter_ctx *ctx, char *buf, FILE *fp) {

  seccomp_release(*ctx);
  free(buf);
  if (fclose(fp) != 0) {
    printErr(__FILE__, __LINE__, "fclose failed", 1, errno);
    return -1;
  }
  return 0;
}

int installSysCallBlocker(scmp_filter_ctx *ctx, int whitelist_fd) {

  FILE *fp = fdopen(whitelist_fd, "r");
  if (fp == NULL) {
    printErr(__FILE__, __LINE__, "fopen failed", 1, errno);
    return -1;
  }

  *ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));
  if (*ctx == NULL) {
    printErr(__FILE__, __LINE__, "seccomp_init failed", 0, 0);
    return -1;
  }

  // Add filters for default whitelist
  const char *def_wl[] = {"exit_group", "execve"};
  int ret, i, def_wl_len = sizeof(def_wl)/sizeof(def_wl[0]);
  for (i = 0; i < def_wl_len; i++) {
    if ((ret = seccomp_rule_add(
      *ctx, SCMP_ACT_ALLOW,
      seccomp_syscall_resolve_name(def_wl[i]), 0)) < 0) {
      printErr(__FILE__, __LINE__, "seccomp_rule_add failed", 1, -ret);
      if (installSysCallBlockerCleanup(ctx, NULL, fp) == -1) {
        printErr(
          __FILE__, __LINE__, "installSysCallBlockerCleanup failed", 0,
          0);
      }
      return -1;
    }
  }

  // Add filters for user's whitelist
  int max_syscall_len = 25;
  char *buf = malloc(sizeof(char) * max_syscall_len);
  // TODO: first call to execl should be allowed
  while (fgets(buf, max_syscall_len, fp) != NULL) {
    buf[strlen(buf) - 1] = '\0';
    if ((ret = seccomp_rule_add(
      *ctx, SCMP_ACT_ALLOW, seccomp_syscall_resolve_name(buf), 0)) < 0) {
      printErr(__FILE__, __LINE__, "seccomp_rule_add failed", 1, -ret);
      if (installSysCallBlockerCleanup(ctx, buf, fp) == -1) {
        printErr(
          __FILE__, __LINE__, "installSysCallBlockerCleanup failed", 0,
          0);
      }
      return -1;
    }
  }

  // Free resources
  free(buf);
  if (fclose(fp) != 0) {
    printErr(__FILE__, __LINE__, "fclose failed", 1, errno);
  }

  // Load the filter
  if ((ret = seccomp_load(*ctx)) < 0) {
    printErr(__FILE__, __LINE__, "seccomp_load failed", 1, -ret);
    return -1;
  }
  return 0;
}