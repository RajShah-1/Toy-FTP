// To Compile: g++ -o ./out/Server Server.cpp -pthread

#include <arpa/inet.h>
#include <dirent.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "common.hpp"

void* handleFTP(void* args);
void handleCommands(int socket_fd, char command[CMD_SIZE],
                    char username[USERNAME_LEN], bool isBinary);
void handleLIST(int socket_fd, char username[USERNAME_LEN]);
void handleGET(int socket_fd, char command[CMD_SIZE],
               char username[USERNAME_LEN], bool isBinary);

int setupSocket(void);
size_t getFileSize(FILE* fptr);
int newRecv(int socket_fd, void* buffer, size_t buffer_size);
void newSend(int socket_fd, const void* buffer, size_t buffer_size);

int main() {
  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof client_addr;
  char ipstr[INET_ADDRSTRLEN];
  pthread_t tid;
  int new_fd;

  int socket_fd = setupSocket();
  printf("Waiting for clients...\n");

  while (1) {
    // Accept new connection
    new_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_size);
    if (new_fd == -1) {
      perror("accept failed\n");
      printf("Ignoring the connection\n");
      continue;
    }
    inet_ntop(client_addr.ss_family,
              &(((struct sockaddr_in*)(&client_addr))->sin_addr), ipstr,
              sizeof ipstr);
    printf("Got connection from %s\n", ipstr);

    pthread_create(&tid, NULL, handleFTP, &new_fd);
    // We don't need to join the thread later
    pthread_detach(tid);
  }

  // clean up
  close(new_fd);
  close(socket_fd);
}

void* handleFTP(void* args) {
  int socket_fd = *(int*)args;
  char command[CMD_SIZE];
  char username[USERNAME_LEN] = "raj";
  bool isBinary = true;
  printf("Connection received\n");
  try {
    while (1) {
      newRecv(socket_fd, command, CMD_SIZE);
      printf("Received: %s\n", command);
      handleCommands(socket_fd, command, username, isBinary);
    }
    return NULL;
  } catch (const char* error_msg) {
    printf("[ERROR] %s\n", error_msg);
  }
}

void handleCommands(int socket_fd, char command[CMD_SIZE],
                    char username[USERNAME_LEN], bool isBinary) {
  char cmd_type[CMD_TYPE_SIZE];
  if (sscanf(command, "%s ", cmd_type) != 1) {
    throw "Invalid command";
  }
  printf("Command Type: %s\n", cmd_type);

  if (strcasecmp(cmd_type, "GET") == 0) {
    handleGET(socket_fd, command, username, isBinary);
  } else if ((strcasecmp(cmd_type, "PUT") == 0)) {
    // handlePUT(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "MODE") == 0)) {
    // handleMODE(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "LIST") == 0)) {
    handleLIST(socket_fd, username);
  } else if ((strcasecmp(cmd_type, "DELETE") == 0)) {
    // handleDEL(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "CHANGE_PASS") == 0)) {
    throw "Functionality not implemented";
  } else if ((strcasecmp(cmd_type, "EXIT") == 0)) {
    exit(EXIT_SUCCESS);
  } else {
    throw "Invalid command";
  }
}

void handleGET(int socket_fd, char command[CMD_SIZE],
               char username[USERNAME_LEN], bool isBinary) {
  char filename[CMD_ARG_SIZE];
  // read the file name and get the file path
  if (sscanf(command, "%*s %s", filename) != 1) {
    throw "Invalid filename";
  }
  std::string filePath =
      ("./Server/" + std::string(username) + "/" + std::string(filename));
  printf("GET %s\n", filePath.c_str());
  // try opening the file
  FILE* fptr;
  if (isBinary)
    fptr = fopen(filePath.c_str(), "rb");
  else
    fptr = fopen(filePath.c_str(), "r");

  if (fptr == NULL) {
    newSend(socket_fd, "NOTFOUND", STATUS_SIZE);
    printf("File not found\n");
    return;
  } else {
    newSend(socket_fd, "FOUND", STATUS_SIZE);
  }

  // send the file size
  size_t fileSize = getFileSize(fptr);
  newSend(socket_fd, &fileSize, sizeof(size_t));
  // start sending the file
  char sendBuffer[BUFFER_SIZE];
  memset(sendBuffer, 0, BUFFER_SIZE);
  size_t numBytes = 0, totBytesSent = 0;
  while (totBytesSent < fileSize) {
    numBytes = fread(sendBuffer, sizeof(char), BUFFER_SIZE, fptr);
    newSend(socket_fd, sendBuffer, numBytes);
    totBytesSent += numBytes;
  }
  printf("File sent successfully\n");
}

void handleLIST(int socket_fd, char username[USERNAME_LEN]) {
  printf("LIST\n");
  // open the dir
  DIR* dir;
  struct dirent* dir_entry;
  std::string dir_path = "./Server/" + std::string(username);
  dir = opendir(dir_path.c_str());
  // make a space seperated list of filenames
  std::string fileNames = "";
  if (dir) {
    while ((dir_entry = readdir(dir)) != NULL) {
      if (strcmp(dir_entry->d_name, ".") == 0 ||
          strcmp(dir_entry->d_name, "..") == 0) {
        continue;
      }
      fileNames += std::string(dir_entry->d_name) + " ";
    }
    closedir(dir);
  }
  const char* fileNamesC = fileNames.c_str();
  size_t numbytes = strlen(fileNamesC) + 1;  // +1 for null termination
  // send the number of characters in the space seperated list of the filenames
  newSend(socket_fd, &numbytes, sizeof(size_t));
  // send filenames for all the files
  newSend(socket_fd, fileNamesC, numbytes);
}

void newSend(int socket_fd, const void* buffer, size_t buffer_size) {
  int status = send(socket_fd, buffer, buffer_size, 0);
  if (status == -1) {
    throw "Send Failed";
  }
}

int newRecv(int socket_fd, void* buffer, size_t buffer_size) {
  int numbytes = recv(socket_fd, buffer, buffer_size, 0);
  if (numbytes == -1) {
    throw "Recv Failed";
  }
  if (numbytes == 0) {
    printf("Connection closed\n");
    printf("Killing the thread...\n");
    pthread_exit(NULL);
  }
  return numbytes;
}

size_t getFileSize(FILE* fptr) {
  size_t fileSize;
  struct stat fileInfo;
  // go to the end of the file
  if (fseek(fptr, 0L, SEEK_END) != 0) {
    throw "fseek failed";
  }
  // read the current position (gives fileSize)
  fileSize = ftell(fptr);
  if (ftell < 0) {
    throw "ftell failed";
  }
  // go back to the start of the file
  rewind(fptr);
  return fileSize;
}

int setupSocket(void) {
  int status;
  int optval_yes = 1;
  int socket_fd;

  struct addrinfo hints, *serv_info;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP based stream socket
  hints.ai_flags =
      AI_PASSIVE;  // get addr suitable for binding (if nodename = NULL)

  // Get addrinfo for localhost (i.e. get loopback IP address)
  if ((status = getaddrinfo(NULL, PORT_NUM, &hints, &serv_info)) != 0) {
    printf("getaddrinfo failed!: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // Initialize socket file descriptor
  socket_fd = socket(serv_info->ai_family, serv_info->ai_socktype,
                     serv_info->ai_protocol);
  fatal_error_guard(socket_fd <= 0, "socket_fd <= 0");

  fatal_error_guard(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval_yes,
                               sizeof(int)) == -1,
                    "setsockopt failed");
  // bind socket to localhost
  fatal_error_guard(
      bind(socket_fd, serv_info->ai_addr, serv_info->ai_addrlen) == -1,
      "bind failed");
  // listen for connection requests
  fatal_error_guard(listen(socket_fd, 5) == -1, "listen failed");

  // serv_info is no longer required
  freeaddrinfo(serv_info);
  return socket_fd;
}
