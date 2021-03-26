#include "./libs/include/Authorization.hpp"
#include "./libs/include/FileTransfer.hpp"

const char* PORT_NUM = "3490";

void* handleFTP(void* args) {
  int socket_fd = *(int*)args;

  int numbytes;
  char command[COMMAND_SIZE];
  char fileName[FILENAME_SIZE];
  char status[STATUS_SIZE];
  FileTransfer ftp(socket_fd, false, false);
  char userName[USERNAME_SIZE];
  char password[PASSWORD_SIZE];
  std::string filePath;
  Authorization auth;

  error_guard((numbytes = recv(socket_fd, userName, USERNAME_SIZE, 0)) == -1,
              "socket receive failed");
  error_guard((numbytes = recv(socket_fd, password, PASSWORD_SIZE, 0)) == -1,
              "socket receive failed");

  if (!auth.authenticate(std::string(userName), std::string(password))) {
    error_guard(send(socket_fd, "FAILED", STATUS_SIZE, 0) == -1, "send failed");
    printf("Wrong credentials\n");
    printf("Terminating the thread...\n");
    pthread_exit(NULL);
  }
  error_guard(send(socket_fd, "SUCCESS", STATUS_SIZE, 0) == -1, "send failed");

  std::string clientName = auth.getCurrentUserName();
  std::string dirPath = "./Server/" + clientName;

  // Handle Auth
  while (1) {
    error_guard((numbytes = recv(socket_fd, command, COMMAND_SIZE, 0)) == -1,
                "socket receive failed");
    if (numbytes == 0) {
      pthread_exit(NULL);
    }
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
      FILE* fptr;
      if (strcasecmp(ftp.getMode(), "B") == 0) {
        fptr = fopen(filePath.c_str(), "rb");
      } else {
        fptr = fopen(filePath.c_str(), "r");
      }
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
    } else if (strcasecmp(command, "MODEB") == 0) {
      ftp.setMode("B");
    } else if (strcasecmp(command, "MODEC") == 0) {
      ftp.setMode("C");
    } else if (strcasecmp(command, "LOGOUT") == 0) {
      auth.logout();
    } else if (strcasecmp(command, "EXIT") == 0) {
      printf("Received exit from the user\n");
      pthread_exit(NULL);
      // return NULL;
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

  printf("Waiting for clients...\n");

  addr_size = sizeof their_addr;
  pthread_t tid;
  while (1) {
    // Accept new connection
    new_fd = accept(socket_fd, (struct sockaddr*)&their_addr, &addr_size);
    if (new_fd == -1) {
      perror("accept failed\n");
      exit(EXIT_FAILURE);
    }
    inet_ntop(their_addr.ss_family,
              &(((struct sockaddr_in*)(&their_addr))->sin_addr), ipstr,
              sizeof ipstr);
    printf("server: got connection from %s\n", ipstr);

    pthread_create(&tid, NULL, handleFTP, &new_fd);
    // We don't need to join the thread lates
    // So, no strings attached!
    pthread_detach(tid);
  }
  // clean up
  close(new_fd);
  close(socket_fd);
}