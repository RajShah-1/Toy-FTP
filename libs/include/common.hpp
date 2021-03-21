#ifndef __COMMON_H
#define __COMMON_H 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

typedef unsigned long filesize_t;

inline static void error_guard(int is_error, const char* err_msg) {
  if (is_error) {
    fprintf(stderr, "%s\n", err_msg);
    exit(EXIT_FAILURE);
  }
}

#endif