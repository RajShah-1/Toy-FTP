#include "./libs/include/FileTransfer.hpp"

const char* PORT_NUM = "3490";
const int COMMAND_SIZE = 100;
const int STATUS_SIZE = 10;

void handleFTP(int socket_fd) {
  std::string clientName = "client_1";
  int numbytes;
  char command[COMMAND_SIZE];
  char fileName[FILENAME_SIZE];
  char status[STATUS_SIZE];
  FileTransfer ftp(socket_fd);
  std::string filePath;
  std::string dirPath = "./Server/" + clientName;

  while (1) {
    error_guard((numbytes = recv(socket_fd, command, COMMAND_SIZE, 0)) == -1,
                "socket receive failed");
    if (strcasecmp(command, "PUT") == 0) {
      // receive the file and store it
      ftp.recvFile(dirPath);
    } else if (strcasecmp(command, "GET") == 0) {
      // receive the name of the requested file
      error_guard(
          (numbytes = recv(socket_fd, fileName, FILENAME_SIZE, 0)) == -1,
          "socket receive failed");
      // send the file
      filePath = dirPath + "/" + std::string(fileName);
      printf("Attempting to open %s\n", filePath.c_str());
      FILE* fptr = fopen(filePath.c_str(), "rb");
      if (fptr == NULL) {
        // File not found
        printf("File not found\n");
        error_guard(send(socket_fd, "NOTFOUND", STATUS_SIZE, 0) == -1,
                    "send failed");

      } else {
        error_guard(send(socket_fd, "FOUND", STATUS_SIZE, 0) == -1,
                    "send failed");
        ftp.sendFile(fptr, fileName);
        fclose(fptr);
      }
    } else if (strcasecmp(command, "EXIT") == 0) {
      return;
    }
  }
}

int main() {
  int status;
  int optval_yes = 1;
  char ipstr[INET_ADDRSTRLEN];
  int socket_fd, new_fd;
  socklen_t addr_size;
  struct sockaddr_storage their_addr;

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
  error_guard(socket_fd <= 0, "socket_fd <= 0");

  error_guard(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval_yes,
                         sizeof(int)) == -1,
              "setsockopt failed");
  // bind socket to localhost
  error_guard(bind(socket_fd, serv_info->ai_addr, serv_info->ai_addrlen) == -1,
              "bind failed");
  // listen for connection requests
  error_guard(listen(socket_fd, 5) == -1, "listen failed");

  // serv_info is no longer required
  freeaddrinfo(serv_info);

  printf("Waiting for the client...\n");

  // Accept new connection
  addr_size = sizeof their_addr;
  new_fd = accept(socket_fd, (struct sockaddr*)&their_addr, &addr_size);
  if (new_fd == -1) {
    perror("accept failed\n");
    exit(EXIT_FAILURE);
  }
  inet_ntop(their_addr.ss_family,
            &(((struct sockaddr_in*)(&their_addr))->sin_addr), ipstr,
            sizeof ipstr);
  printf("server: got connection from %s\n", ipstr);

  handleFTP(new_fd);
  // clean up
  close(new_fd);
  close(socket_fd);
}