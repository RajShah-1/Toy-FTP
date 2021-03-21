#ifndef __FILE_TRANSFER_H
#define __FILE_TRANSFER_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.hpp"

const int BUFFER_SIZE = 1024;
const int FILENAME_SIZE = 100;
const int WAITING_TIME = 3;

class FileTransfer {
  int socket_fd;
  filesize_t get_file_size(FILE* fptr);

 public:
  FileTransfer(int socket_fd);
  void recvFile(std::string destFolder);
  void sendFile(std::string fileName, std::string srcFolder);
};

#endif