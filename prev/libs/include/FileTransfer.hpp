#ifndef __FILE_TRANSFER_H
#define __FILE_TRANSFER_H

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.hpp"

const int BUFFER_SIZE = 1024;
const int FILENAME_SIZE = 100;
const int WAITING_TIME = 3;
const int STATUS_SIZE = 10;
const int COMMAND_SIZE = 100;
const int PROMPT_SIZE = 50;

class FileTransfer {
  int socket_fd;
  bool isBinary;
  bool isClient;
  bool printProgress;
  filesize_t getFileSize(FILE* fptr);
  std::string getFileName(std::string filePath);
  void print_progress(unsigned int finished, unsigned int total);

 public:
  FileTransfer(int socket_fd, bool printProgress, bool isClient);
  void recvFile(std::string destFolder);
  void sendFile(FILE* fptr, std::string fileName);
  void setMode(const char mode[1]);
  const char* getMode(void);
};

#endif