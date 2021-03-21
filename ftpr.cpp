#include "./libs/include/FileTransfer.hpp"

const char* PORT_NUM = "3490";

void handle_ftp(int socket_fd) {
  std::string clientName = "client_1";
  // receive command from the client
  // while(1){

  // }
  FileTransfer ftp(socket_fd);
  ftp.recvFile("./Server/"+clientName);
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

  handle_ftp(new_fd);
  // clean up
  close(new_fd);
  close(socket_fd);
}