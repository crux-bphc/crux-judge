#include <stdio.h>

void printErr(
  const char *file, int line, const char *msg, int print_errno, int num) {
  if (print_errno) {
    fprintf(stderr, "%s: %d: %s: errno: %d\n", file, line, msg, num);
  }
  else {
    fprintf(stderr, "%s: %d: %s\n", file, line, msg);
  }
}