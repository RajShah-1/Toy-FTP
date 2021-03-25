#include "./libs/include/Authorization.hpp"
#include "./libs/include/FileTransfer.hpp"

const char* PORT_NUM = "3490";
const int COMMAND_SIZE = 100;
const int STATUS_SIZE = 10;

void readString(char* buffer, int bufferSize) {
  fgets(buffer, bufferSize, stdin);

  if ((strlen(buffer) > 0) && (buffer[strlen(buffer) - 1] == '\n'))
    buffer[strlen(buffer) - 1] = '\0';
}

void handleFTP(int socket_fd) {
  FileTransfer ftp(socket_fd);
  char fileName[FILENAME_SIZE];
  char status[STATUS_SIZE];
  char command[COMMAND_SIZE];
  char userName[USERNAME_SIZE];
  char password[PASSWORD_SIZE];
  int numbytes;
  bool isASCII = false;
  // if isASCII = true -> Character mode
  // if isASCII = false -> Binary mode

  // Handle Auth
  printf("Username: ");
  readString(userName, USERNAME_SIZE);
  printf("Password: ");
  readString(password, PASSWORD_SIZE);

  // send the username and the password to the server
  error_guard(send(socket_fd, userName, USERNAME_SIZE, 0) == -1, "send failed");
  error_guard(send(socket_fd, password, PASSWORD_SIZE, 0) == -1, "send failed");

  error_guard((numbytes = recv(socket_fd, status, STATUS_SIZE, 0)) == -1,
              "receive failed");

  if (strcmp("SUCCESS", status) != 0) {
    printf("Authentication failed!\n");
    printf("Terminating...\n");
    return;
  }

  printf("Logged in...\n");
  printf("Transfer mode: Binary\n");

  while (1) {
    printf("Enter command: ");
    readString(command, COMMAND_SIZE);
    printf("Entered: %s\n", command);

    if (strcasecmp(command, "PUT") == 0) {
      // ask for the file name
      printf("Enter file name: ");
      readString(fileName, FILENAME_SIZE);
      FILE* fptr = fopen(fileName, "rb");
      if (fptr == NULL) {
        printf("Unable to open the file. Try again.\n");
        continue;
      }
      // send the command to the server
      error_guard(send(socket_fd, command, COMMAND_SIZE, 0) == -1,
                  "send failed");
      printf("Sending %s to the server...\n", fileName);
      ftp.sendFile(fptr, std::string(fileName));
      fclose(fptr);
    } else if (strcasecmp(command, "GET") == 0) {
      printf("Enter file name: ");
      readString(fileName, FILENAME_SIZE);
      printf("Requesting %s from server...\n", fileName);
      error_guard(send(socket_fd, command, COMMAND_SIZE, 0) == -1,
                  "send failed");
      error_guard(send(socket_fd, fileName, FILENAME_SIZE, 0) == -1,
                  "send failed");
      error_guard((numbytes = recv(socket_fd, status, STATUS_SIZE , 0)) == -1,
                  "receive failed");
      if (strcasecmp(status, "FOUND") == 0) {
        ftp.recvFile(".");
      } else {
        printf("File not available on the server\n");
        continue;
      }
    } else if (strcasecmp(command, "EXIT") == 0) {
      // send the command to the server
      error_guard(send(socket_fd, command, COMMAND_SIZE, 0) == -1,
                  "send failed");
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
  handleFTP(socket_fd);
  // Close socket file descriptior
  close(socket_fd);
}