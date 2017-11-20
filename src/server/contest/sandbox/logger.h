#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

#define SB_PRINT_STATS

#define SB_PRINT_ERR
#define SB_PRINT_RESULT
//#define SB_PRINT_DEBUG

#ifdef SB_PRINT_ERR
  #define SB_PRINT_ERR_TEST 1
#else
  #define SB_PRINT_ERR_TEST 0
#endif

#ifdef SB_PRINT_RESULT
  #define SB_PRINT_RESULT_TEST 1
#else
  #define SB_PRINT_RESULT_TEST 0
#endif

#ifdef SB_PRINT_DEBUG
  #define SB_PRINT_DEBUG_TEST 1
#else
  #define SB_PRINT_DEBUG_TEST 0
#endif

#define printErr(...) \
do { \
  if (SB_PRINT_ERR_TEST) { \
    fprintf(stderr, "%s: %d: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } \
} while (0)

#define printResult(...) \
do { \
  if (SB_PRINT_RESULT_TEST) { \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n"); \
  } \
} while (0)

#define printDebug(...) \
do { \
  if (SB_PRINT_DEBUG_TEST) { \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } \
} while (0)

#endif