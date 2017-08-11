#include <stdlib.h>
#include <stdio.h>

#include "sandbox.h"

int main(int argc, char *argv[]) {

  ResLimits r;
  CgroupLocs c;

  r.mem = argv[1];
  r.cpu_time = argv[2];
  r.num_tasks = argv[3];
  c.memory = argv[4];
  c.cpuacct = argv[5];
  c.pids = argv[6];
  const char *jail_path = argv[7];
  // 'exect_path' must be given relative to 'jail_path'
  const char *exect_path = argv[8];
  const char *input_file = argv[9];
  const char *output_file = argv[10];
  // whitelist file should have a newline after the last word
  const char *whitelist = argv[11];
  uid_t uid = atoi(argv[12]);
  gid_t gid = atoi(argv[13]);

  return sandboxExec(
    exect_path, jail_path, 
    input_file, output_file, &c, &r,
    whitelist, uid, gid);
}