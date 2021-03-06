#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.hpp"

const int PATH_MAX = 100;

int setupSocket(void);
void readCMD(char* buffer, int bufferSize);
void newSend(int socket_fd, const void* buffer, size_t buffer_size);
int newRecv(int socket_fd, void* buffer, size_t buffer_size);
size_t getFileSize(FILE* fptr);
void printProgress(unsigned long eFinished, unsigned long eTotal);
bool authenticate(int socket_fd, char username[USERNAME_LEN],
                  char password[PASSWORD_LEN]);
void cd(char command[CMD_SIZE]);

void handleFTP(int socket_fd);
void handleCommands(int socket_fd, char command[CMD_SIZE], bool& isBinary);
void handleLIST(int socket_fd, char command[CMD_SIZE]);
void handleGET(int socket_fd, char command[CMD_SIZE], bool isBinary);
void handlePUT(int socket_fd, char command[CMD_SIZE], bool isBinary);
void handleMODE(int socket_fd, char command[CMD_SIZE], bool& isBinary);
void handleDEL(int socket_fd, char command[CMD_SIZE]);

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
  bool isBinary = true;
  printf("Connected Successfully!\n");
  // client authentication
  char username[USERNAME_LEN];
  char password[PASSWORD_LEN];

  bool isAuthSuccess = authenticate(socket_fd, username, password);
  while (!isAuthSuccess) {
    printf("Authentication failed\n");
    printf("Try again\n");
    isAuthSuccess = authenticate(socket_fd, username, password);
  }
  printf("Logged in as %s\n", username);

  while (1) {
    readCMD(command, CMD_SIZE);
    try {
      handleCommands(socket_fd, command, isBinary);
    } catch (const char* error_msg) {
      printf("[ERROR] %s\n", error_msg);
    }
    memset(command, 0, CMD_SIZE);
  }
}

bool authenticate(int socket_fd, char username[USERNAME_LEN],
                  char password[PASSWORD_LEN]) {
  printf("User name: ");
  scanf(" %s", username);
  printf("Password: ");
  scanf(" %s", password);

  newSend(socket_fd, username, USERNAME_LEN);
  newSend(socket_fd, password, PASSWORD_LEN);

  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  if ((strcasecmp(status, "ACTIVE") == 0)) {
    printf("User is already active\n");
  }
  return (strcasecmp(status, "SUCCESS") == 0);
}

void handleCommands(int socket_fd, char command[CMD_SIZE], bool& isBinary) {
  char cmd_type[CMD_TYPE_SIZE];
  if (sscanf(command, "%s ", cmd_type) != 1) {
    throw "Invalid command";
  }
  // printf("Command Type: %s\n", cmd_type);

  if (strcasecmp(cmd_type, "GET") == 0) {
    handleGET(socket_fd, command, isBinary);
  } else if ((strcasecmp(cmd_type, "PUT") == 0)) {
    handlePUT(socket_fd, command, isBinary);
  } else if ((strcasecmp(cmd_type, "MODE") == 0)) {
    handleMODE(socket_fd, command, isBinary);
  } else if ((strcasecmp(cmd_type, "LIST") == 0)) {
    handleLIST(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "DELETE") == 0)) {
    handleDEL(socket_fd, command);
  } else if ((strcasecmp(cmd_type, "CD") == 0)) {
    cd(command);
  } else if ((strcasecmp(cmd_type, "LS") == 0)) {
    system("ls");
  } else if ((strcasecmp(cmd_type, "EXIT") == 0)) {
    exit(EXIT_SUCCESS);
  } else {
    throw "Invalid command";
  }
}

void handleLIST(int socket_fd, char command[CMD_SIZE]) {
  // send the command LIST to the server
  newSend(socket_fd, command, CMD_SIZE);
  // receive the number of files present in the dir
  size_t numBytes;
  newRecv(socket_fd, &numBytes, sizeof(size_t));
  // receive all the file names and file sizes
  char* filenames = (char*)malloc(numBytes);
  size_t totalBytesRcvd = 0;
  while (totalBytesRcvd < numBytes) {
    size_t numRcvd = newRecv(socket_fd, filenames + totalBytesRcvd, numBytes);
    totalBytesRcvd += numRcvd;
  }
  // print filenames
  char* filename = strtok(filenames, " ");
  printf("Files:\n");
  while (filename != NULL) {
    printf("\t%s\n", filename);
    filename = strtok(NULL, " ");
  }
  free(filenames);
}

void handlePUT(int socket_fd, char command[CMD_SIZE], bool isBinary) {
  char filename[CMD_ARG_SIZE];
  if (sscanf(command, "%*s %s", filename) != 1) {
    throw "Invalid filename";
  }
  // check if the file exist or not
  bool isFileAvl = (access(filename, F_OK) != 0);
  if (isFileAvl) {
    throw "File does not exist";
  }
  // Attempt to open the file
  printf("Attempting to open %s\n", filename);
  FILE* fptr;
  if (isBinary)
    fptr = fopen(filename, "rb");
  else
    fptr = fopen(filename, "r");

  if (fptr == NULL) throw "Unable to create the file";

  // Send "PUT" and the file name
  newSend(socket_fd, command, CMD_SIZE);

  // receive the status from the server
  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  if (strcasecmp(status, "FOUND") == 0) {
    // if the file is already present on the server
    // ask user whether to abort/overwrite/append
    printf("The file already exists on the server\n");
    printf("Enter [Abort/Overwrite/Append]: ");
    char option[STATUS_SIZE];
    scanf(" %s", option);

    if (strcasecmp(option, "ABORT") == 0 ||
        strcasecmp(option, "OVERWRITE") == 0 ||
        strcasecmp(option, "APPEND") == 0) {
      if (strcasecmp(option, "APPEND") == 0 && isBinary) {
        printf("Append is not supported in binary mode\n");
        printf("Aborting...\n");
        newSend(socket_fd, "ABORT", STATUS_SIZE);
        fclose(fptr);
        return;
      }
      newSend(socket_fd, option, STATUS_SIZE);
      if (strcasecmp(option, "ABORT") == 0) {
        fclose(fptr);
        return;
      }
    } else {
      newSend(socket_fd, "ABORT", STATUS_SIZE);
      printf("Invalid option\n");
      return;
    }
  }
  // send the file size
  size_t fileSize = getFileSize(fptr);
  newSend(socket_fd, &fileSize, sizeof(size_t));
  // send the file
  char sendBuffer[BUFFER_SIZE];
  memset(sendBuffer, 0, BUFFER_SIZE);
  size_t numBytes = 0, totBytesSent = 0;
  while (totBytesSent < fileSize) {
    numBytes = fread(sendBuffer, sizeof(char), BUFFER_SIZE, fptr);
    if (numBytes < 0) throw "fread failed";
    newSend(socket_fd, sendBuffer, numBytes);
    totBytesSent += numBytes;
    printProgress(totBytesSent, fileSize);
  }
  printf("\n");
  printf("File sent successfully\n");
}

void handleGET(int socket_fd, char command[CMD_SIZE], bool isBinary) {
  char filename[CMD_ARG_SIZE];
  if (sscanf(command, "%*s %s", filename) != 1) {
    throw "Invalid filename";
  }
  // check if the file creation is possible
  FILE* fptr;
  // check if we need to create a new file
  bool isNewFileCreated = (access(filename, F_OK) != 0);
  if (isBinary)
    fptr = fopen(filename, "wb");
  else
    fptr = fopen(filename, "w");

  if (fptr == NULL) throw "Unable to create the file";

  // Send "GET" and the file name
  newSend(socket_fd, command, CMD_SIZE);
  // receive the stauts from the Server (FOUND/NOT_FOUND)
  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  // if the status is not "FOUND", throw an error
  if (strcasecmp(status, "FOUND") != 0) {
    fclose(fptr);
    // if the file is not found and we had to create a new file
    // remove the newly created file
    if (isNewFileCreated) {
      remove(filename);
    }
    throw "File not found";
  }

  // receive the file-size
  size_t filesize;
  char recvBuffer[BUFFER_SIZE];
  newRecv(socket_fd, &filesize, sizeof(size_t));
  printf("Receiving %lu bytes...\n", filesize);

  // receive the file packet by packet
  size_t totBytesRcvd = 0, numBytes = 0;
  while (totBytesRcvd < filesize) {
    numBytes = newRecv(socket_fd, recvBuffer,
                       std::min((size_t)BUFFER_SIZE, filesize - totBytesRcvd));
    if (fwrite(recvBuffer, sizeof(char), numBytes, fptr) != numBytes) {
      fclose(fptr);
      throw "Write error";
    }
    totBytesRcvd += numBytes;
    printProgress(totBytesRcvd, filesize);
  }
  printf("\n");

  // close the fd
  fclose(fptr);
}

void handleMODE(int socket_fd, char command[CMD_SIZE], bool& isBinary) {
  char modeOption[STATUS_SIZE];
  if (sscanf(command, "%*s %s", modeOption) != 1) {
    throw "Invalid mode option";
  }
  if (strcasecmp(modeOption, "BIN") == 0) {
    printf("Transfer mode: Binary\n");
    newSend(socket_fd, command, CMD_SIZE);
    isBinary = true;
  } else if (strcasecmp(modeOption, "CHAR") == 0) {
    newSend(socket_fd, command, CMD_SIZE);
    printf("Transfer mode: character\n");
    isBinary = false;
  } else {
    throw "Invalid mode option";
  }
}

void handleDEL(int socket_fd, char command[CMD_SIZE]) {
  char fileName[CMD_ARG_SIZE];
  if (sscanf(command, "%*s %s", fileName) != 1) {
    throw "Invalid filename";
  }
  // send the command to the server
  newSend(socket_fd, command, CMD_SIZE);
  // recv status from the server
  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  if (strcasecmp(status, "FOUND") != 0) {
    printf("File is not availble on the server\n");
    return;
  }
  // confirmation
  printf("Enter yes to delete %s: ", fileName);
  scanf(" %s", status);
  if (strcasecmp(status, "YES") == 0) {
    newSend(socket_fd, "DEL_ACK", STATUS_SIZE);
    newRecv(socket_fd, status, STATUS_SIZE);
    if (strcasecmp(status, "SUCCESS") == 0) {
      printf("%s deleted successfully\n", fileName);
    } else {
      printf("Deletion failed\n");
    }
  } else {
    newSend(socket_fd, "ABORT", STATUS_SIZE);
    printf("Aborting delete...\n");
  }
}

void newSend(int socket_fd, const void* buffer, size_t buffer_size) {
  int status = send(socket_fd, buffer, buffer_size, 0);
  if (status == -1) {
    throw "Send Failed";
  }
}

int newRecv(int socket_fd, void* buffer, size_t buffer_size) {
  int numBytes = recv(socket_fd, buffer, buffer_size, 0);
  if (numBytes == -1) {
    throw "Recv Failed";
  }
  if (numBytes == 0) {
    printf("Connection closed\n");
    printf("Terminating...\n");
    exit(EXIT_FAILURE);
  }
  return numBytes;
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

// Taking this snippet from my Basic Shell assignment
// change directory
// Supports both absolute and relative paths!
// No spaces allowed
void cd(char command[CMD_SIZE]) {
  char argPath[PATH_MAX];
  if (sscanf(command, "%*s %s", argPath) != 1) {
    printf("Enter a valid path!\n");
    return;
  }
  if (argPath == NULL) {
    printf("Enter a valid path!\n");
    return;
  }

  char path[PATH_MAX];
  strcpy(path, argPath);

  int status = 0;
  if (argPath[0] != '/') {
    // Handle relative paths
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == NULL) {
      printf("Error while obtaining cwd\n");
      return;
    }
    strcat(cwd, "/");
    strcat(cwd, path);
    status = chdir(cwd);
  } else {
    // Handle absolute paths
    status = chdir(argPath);
  }
  if (status == -1) {
    printf("Enter a valid path!\n");
  }
}

void printPrompt(void) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, PATH_MAX) == NULL) {
    printf("Error while obtaining cwd\n");
    printf(
        "\033[1;32m"
        "\r$"
        "\033[0m");
  } else {
    printf(
        "\033[1;32m"
        "\r%s$ "
        "\033[0m",
        cwd);
  }
}

void readCMD(char* buffer, int bufferSize) {
  printPrompt();
  fgets(buffer, bufferSize, stdin);

  if ((strlen(buffer) > 0) && (buffer[strlen(buffer) - 1] == '\n'))
    buffer[strlen(buffer) - 1] = '\0';
  if (strlen(buffer) == 0) {
    // retry
    readCMD(buffer, bufferSize);
  }
}

// References:
// https://stackoverflow.com/questions/1508490/erase-the-current-printed-console-line/1508589#1508589
// https://stackoverflow.com/questions/338273/why-does-printf-not-print-anything-before-sleep/338295#338295
void printProgress(unsigned long eFinished, unsigned long eTotal) {
  struct winsize windowSize;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize);
  unsigned long promptSize = std::max((int)(0.9 * windowSize.ws_col - 2), 0);
  unsigned long finished = (promptSize * eFinished) / eTotal;
  unsigned long total = promptSize;

  char buffer[promptSize + 5];
  buffer[0] = '[';
  for (unsigned int i = 1; i <= total; ++i) {
    if (i <= finished - 1 && finished >= 1) {
      buffer[i] = '=';
    } else if (i == finished) {
      buffer[i] = '>';
    } else {
      buffer[i] = ' ';
    }
  }
  buffer[promptSize + 1] = ']';
  buffer[promptSize + 2] = '\0';
  printf("\r%s", buffer);
  fflush(stdout);
}