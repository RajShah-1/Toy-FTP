#ifndef __CONSTANTS_H_
#define __CONSTANTS_H_
#include <stdarg.h>

const char* PORT_NUM = "3490";
const int BUFFER_SIZE = 100;
const int WAITING_TIME = 3;  // in seconds (for client)
const int CMD_TYPE_SIZE = 20;
const int CMD_ARG_SIZE = 80;
const int CMD_SIZE = CMD_ARG_SIZE + CMD_TYPE_SIZE;
const int USERNAME_LEN = 30;
const int STATUS_SIZE = 10;

inline static void write_log(char userName[USERNAME_LEN], const char* format,
                             ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

inline static void fatal_error_guard(int is_error, const char* err_msg) {
  if (is_error) {
    fprintf(stderr, "%s\n", err_msg);
    exit(EXIT_FAILURE);
  }
}
#endif