#include "./libs/include/FileTransfer.hpp"

const char* PORT_NUM = "3490";
const int COMMAND_SIZE = 100;

void readString(char* buffer, int bufferSize) {
  fgets(buffer, bufferSize, stdin);

  if ((strlen(buffer) > 0) && (buffer[strlen(buffer) - 1] == '\n'))
    buffer[strlen(buffer) - 1] = '\0';
}

void handle_ftp(int socket_fd) {
  FileTransfer ftp(socket_fd);
  char fileName[FILENAME_SIZE];
  char command[COMMAND_SIZE];
  while (1) {
    printf("Enter command: ");
    readString(command, COMMAND_SIZE);
    printf("Entered: %s\n", command);

    if (strcasecmp(command, "PUT") == 0) {
      printf("Enter file name: ");
      readString(fileName, FILENAME_SIZE);
      printf("Sending %s to the server...\n", fileName);
      ftp.sendFile(std::string(fileName), ".");
    } else if (strcasecmp(command, "GET") == 0) {
      printf("Enter file name: ");
      readString(fileName, FILENAME_SIZE);
      printf("Requesting %s from server...\n", fileName);
      ftp.recvFile("");
    } else if (strcasecmp(command, "EXIT") == 0) {
      return;
    }
  }
}

int main() {
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
  handle_ftp(socket_fd);
  // Close socket file descriptior
  close(socket_fd);
}