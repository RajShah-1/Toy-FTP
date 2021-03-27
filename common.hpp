#ifndef __CONSTANTS_H_
#define __CONSTANTS_H_
#include <stdarg.h>
#include <sys/stat.h>

#include <algorithm>

const char* PORT_NUM = "3490";
const int BUFFER_SIZE = 100;
const int WAITING_TIME = 3;  // in seconds (for client)
const int CMD_TYPE_SIZE = 20;
const int CMD_ARG_SIZE = 80;
const int CMD_SIZE = CMD_ARG_SIZE + CMD_TYPE_SIZE;
const int USERNAME_LEN = 30;
const int PASSWORD_LEN = 30;
const int STATUS_SIZE = 10;

inline static void fatal_error_guard(int is_error, const char* err_msg) {
  if (is_error) {
    fprintf(stderr, "%s\n", err_msg);
    exit(EXIT_FAILURE);
  }
}
#endif