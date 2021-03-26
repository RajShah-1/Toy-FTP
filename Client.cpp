#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.hpp"

int setupSocket(void);
void readString(char* buffer, int bufferSize);
void newSend(int socket_fd, char* buffer, size_t buffer_size);
void newRecv(int socket_fd, char* buffer, size_t buffer_size);

void handleFTP(int socket_fd);
void handleCommands(int socket_fd, char command[CMD_SIZE]);
void handleGET(int socket_fd, char command[CMD_SIZE]);

int main() {
  // setup the connection
  int socket_fd = setupSocket();
  // handle the FTP requests
  handleFTP(socket_fd);
  // Close socket file descriptior
  close(socket_fd);
}

void handleFTP(int socket_fd) {
  char command[CMD_SIZE];
  printf("Connection Success!\n");

  while (1) {
    printf(">> ");
    readString(command, CMD_SIZE);
    printf("Entered: %s\n", command);
    try {
      handleCommands(socket_fd, command);
    } catch (const char* error_msg) {
      printf("[ERROR] %s\n", error_msg);
    }
    memset(command, 0, CMD_SIZE);
  }
}

void handleCommands(int socket_fd, char command[CMD_SIZE]) {
  char cmd_type[CMD_TYPE_SIZE];
  if (sscanf(command, "%s ", cmd_type) != 1) {
    throw "Invalid command";
  }
  printf("Command Type: %s\n", cmd_type);

  if (strcasecmp(cmd_type, "GET") == 0) {
    handleGET(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "PUT") == 0)) {
    // handlePUT(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "MODE") == 0)) {
    // handleMODE(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "LIST") == 0)) {
    // handleLIST(socket_fd, command);
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

void handleLIST(int socket_fd, char command[CMD_SIZE]) {
  // send the command LIST to the server
  newSend(socket_fd, command, CMD_SIZE);
  // 
}

void handleGET(int socket_fd, char command[CMD_SIZE]) {
  // Send "GET" and the file name
  newSend(socket_fd, command, CMD_SIZE);
  // receive the stauts from the Server (FOUND/NOT_FOUND)
  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  // if the status is not "FOUND", throw an error
  if (strcasecmp(status, "FOUND") != 0) {
    throw "File not found";
    return;
  }
  // receive the file-size

  // receive the file packet by packet
}

void newSend(int socket_fd, char* buffer, size_t buffer_size) {
  int status = send(socket_fd, buffer, buffer_size, 0);
  if (status == -1) {
    throw "Send Failed";
  }
}

void newRecv(int socket_fd, char* buffer, size_t buffer_size) {
  int status = recv(socket_fd, buffer, buffer_size, 0);
  if (status == -1) {
    throw "Recv Failed";
  }
}

int setupSocket(void) {
  int status;
  int socket_fd;
  char ipstr[INET_ADDRSTRLEN];

  struct addrinfo hints, *serv_info;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP based stream socket

  // Get addrinfo for localhost (i.e. get loopback IP address)
  if ((status = getaddrinfo("localhost", PORT_NUM, &hints, &serv_info)) != 0) {
    fprintf(stderr, "getaddrinfo failed!: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // Initialize socket file descriptor
  socket_fd = socket(serv_info->ai_family, serv_info->ai_socktype,
                     serv_info->ai_protocol);
  if (socket_fd <= 0) {
    fprintf(stderr, "Socket failed! socket_fd:%d\n", socket_fd);
    exit(EXIT_FAILURE);
  }

  printf("Waiting for the server to start....\n");
  // Keep on looping until the connection is established
  while (1) {
    // Attempt to connect to the server
    if (connect(socket_fd, serv_info->ai_addr, serv_info->ai_addrlen) == -1) {
      printf("unable to connect! retrying...\n");
      sleep(WAITING_TIME);
    } else {
      // Connection is successful
      break;
    }
  }
  // serv_info is no longer required
  freeaddrinfo(serv_info);
  return socket_fd;
}

void readString(char* buffer, int bufferSize) {
  fgets(buffer, bufferSize, stdin);

  if ((strlen(buffer) > 0) && (buffer[strlen(buffer) - 1] == '\n'))
    buffer[strlen(buffer) - 1] = '\0';
}
