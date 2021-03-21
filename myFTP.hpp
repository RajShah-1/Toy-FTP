#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

// using namespace std;

const char* PORT_NUM = "3490";
const int BUFFER_SIZE = 1024;
const int FILENAME_SIZE = 100;
const int WAITING_TIME = 3;

typedef unsigned long filesize_t;

inline static void error_guard(int is_error, const char* err_msg) {
  if (is_error) {
    fprintf(stderr, "%s\n", err_msg);
    exit(EXIT_FAILURE);
  }
}