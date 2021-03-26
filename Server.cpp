#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.hpp"

void* handleFTP(void* args);

int setupSocket(void);
int newRecv(int socket_fd, char* buffer, size_t buffer_size); 
void newSend(int socket_fd, char* buffer, size_t buffer_size);

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
  printf("Connection received\n");
  try {
    while (1) {
      newRecv(socket_fd, command, CMD_SIZE);
      printf("Received: %s\n", command);
    }
    return NULL;
  } catch (const char* error_msg) {
    printf("[ERROR] %s\n", error_msg);
  }
}

void newSend(int socket_fd, char* buffer, size_t buffer_size) {
  int status = send(socket_fd, buffer, buffer_size, 0);
  if (status == -1) {
    throw "Send Failed";
  }
}

int newRecv(int socket_fd, char* buffer, size_t buffer_size) {
  int numbytes = recv(socket_fd, buffer, buffer_size, 0);
  if (numbytes == -1) {
    throw "Recv Failed";
  } 
  if (numbytes == 0){
    printf("Connection closed\n");
    printf("Killing the thread...\n");
    pthread_exit(NULL);
  }
  return numbytes;
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
