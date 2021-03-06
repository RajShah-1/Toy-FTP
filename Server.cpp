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
#include <unordered_set>

#include "common.hpp"

std::unordered_set<std::string> activeUsers;

class UserLogger {
  std::string userName;
  FILE* logPtr;

 public:
  UserLogger(char userNameC[USERNAME_LEN]) {
    this->userName = std::string(userNameC);
    const time_t timerTime = time(0);
    std::string filePath = "./Server/Logs/" + this->userName + "/" +
                           std::to_string(timerTime) + ".txt";
    printf("opening %s\n", filePath.c_str());
    logPtr = fopen(filePath.c_str(), "w");
    if (logPtr == NULL) {
      printf("Unable to open the log file\n");
      printf("Server will continue to serve without logging to a file\n");
    }
    // add user to the activeUsers set
    activeUsers.insert(this->userName);
    writeLog("User logged in\n");
  }

  ~UserLogger() {
    writeLog("Terminated\n");
    // remover user from the activeUsers set
    activeUsers.erase(this->userName);
    fclose(logPtr);
  }

  const char* getUserNameC(void) { return this->userName.c_str(); }
  std::string getUserName(void) { return this->userName; }

  void writeLog(const char* format, ...) {
    const time_t timerTime = time(0);
    struct tm* currTime = localtime(&timerTime);
    char dateStr[100];
    sprintf(dateStr, "%02d/%02d/%04d %02d:%02d:%02d", currTime->tm_mday,
            1 + currTime->tm_mon, 1900 + currTime->tm_year, currTime->tm_hour,
            currTime->tm_min, currTime->tm_sec);

    va_list args;
    printf("[%s] [%s] ", userName.c_str(), dateStr);
    if (this->logPtr != NULL) {
      fprintf(logPtr, "[%s] [%s] ", userName.c_str(), dateStr);
    }
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    if (this->logPtr != NULL) {
      va_start(args, format);
      vfprintf(logPtr, format, args);
      va_end(args);
    }
  }
};

void* handleFTP(void* args);
void handleCommands(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
                    bool& isBinary);
void handleLIST(int socket_fd, UserLogger& currUser);
void handleGET(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
               bool isBinary);
void handlePUT(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
               bool isBinary);
void handleMODE(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
                bool& isBinary);
void handleDEL(int socket_fd, char command[CMD_SIZE], UserLogger& currUser);

int setupSocket(void);
size_t getFileSize(FILE* fptr);
std::string sanitizeFileName(std::string filePath);
int newRecv(int socket_fd, void* buffer, size_t buffer_size);
void newSend(int socket_fd, const void* buffer, size_t buffer_size);
bool authenticate(int socket_fd, char username[USERNAME_LEN],
                  char password[PASSWORD_LEN]);

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
  char username[USERNAME_LEN];
  char password[PASSWORD_LEN];
  bool isBinary = true;
  bool isLoggedIn = false;

  while (!isLoggedIn) {
    try {
      isLoggedIn = authenticate(socket_fd, username, password);
    } catch (const char* error_msg) {
      printf("[AUTH ERROR] %s\n", error_msg);
    }
  }
  printf("%s logged in\n", username);
  UserLogger currUser(username);

  try {
    while (1) {
      newRecv(socket_fd, command, CMD_SIZE);
      printf("Received: %s\n", command);
      handleCommands(socket_fd, command, currUser, isBinary);
    }
    return NULL;
  } catch (const char* error_msg) {
    printf("[ERROR] %s\n", error_msg);
  }
}

bool authenticate(int socket_fd, char username[USERNAME_LEN],
                  char password[PASSWORD_LEN]) {
  // fetch the list of users and passwords from the file
  newRecv(socket_fd, username, USERNAME_LEN);
  newRecv(socket_fd, password, PASSWORD_LEN);

  const char* authFile = "./Server/auth.txt";
  FILE* authFptr = fopen(authFile, "r");
  if (authFptr == NULL) {
    printf("Unable to open auth.txt");
    pthread_exit(NULL);
  }
  // printf("USERS:\n");
  char authUsr[USERNAME_LEN], authPass[PASSWORD_LEN];
  while (fscanf(authFptr, " %s %s", authUsr, authPass) == 2) {
    // printf("Entry: %s %s\n", authUsr, authPass);
    if (strcmp(authUsr, username) == 0) {
      if (strcmp(password, authPass) == 0) {
        fclose(authFptr);
        if (activeUsers.find(std::string(username)) == activeUsers.end()) {
          newSend(socket_fd, "SUCCESS", STATUS_SIZE);
          return true;
        }
        newSend(socket_fd, "ACTIVE", STATUS_SIZE);
        return false;
      }
    }
  }
  newSend(socket_fd, "DENIED", STATUS_SIZE);
  fclose(authFptr);
  return false;
}

void handleCommands(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
                    bool& isBinary) {
  char cmd_type[CMD_TYPE_SIZE];
  if (sscanf(command, "%s ", cmd_type) != 1) {
    throw "Invalid command";
  }
  // printf("Command Type: %s\n", cmd_type);

  if (strcasecmp(cmd_type, "GET") == 0) {
    handleGET(socket_fd, command, currUser, isBinary);
  } else if ((strcasecmp(cmd_type, "PUT") == 0)) {
    handlePUT(socket_fd, command, currUser, isBinary);
  } else if ((strcasecmp(cmd_type, "MODE") == 0)) {
    handleMODE(socket_fd, command, currUser, isBinary);
  } else if ((strcasecmp(cmd_type, "LIST") == 0)) {
    handleLIST(socket_fd, currUser);
  } else if ((strcasecmp(cmd_type, "DELETE") == 0)) {
    handleDEL(socket_fd, command, currUser);
  } else if ((strcasecmp(cmd_type, "CHANGE_PASS") == 0)) {
    throw "Functionality not implemented";
  } else if ((strcasecmp(cmd_type, "EXIT") == 0)) {
    exit(EXIT_SUCCESS);
  } else {
    throw "Invalid command";
  }
}

void handleLIST(int socket_fd, UserLogger& currUser) {
  currUser.writeLog("LIST\n");
  // open the dir
  DIR* dir;
  struct dirent* dir_entry;
  std::string dir_path = "./Server/" + currUser.getUserName();
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

void handlePUT(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
               bool isBinary) {
  char argFileName[CMD_ARG_SIZE];
  if (sscanf(command, "%*s %s", argFileName) != 1) {
    throw "Invalid filename";
  }
  // check if the file creation is possible
  FILE* fptr;

  std::string filePath = ("./Server/" + currUser.getUserName() + "/" +
                          std::string(sanitizeFileName(argFileName)));
  const char* filePathC = filePath.c_str();
  // check if we need to create a new file
  bool isFilePresent = (access(filePathC, F_OK) == 0);
  bool isAppend = false;
  if (isFilePresent) {
    // send NOTFOUND
    newSend(socket_fd, "FOUND", STATUS_SIZE);
    // recv the option from the user
    char option[STATUS_SIZE];
    newRecv(socket_fd, option, STATUS_SIZE);
    if (strcasecmp(option, "ABORT") == 0) {
      currUser.writeLog("PUT conflict -> Abort\n");
      return;
    }
    if (strcasecmp(option, "APPEND") == 0 && !isBinary) {
      currUser.writeLog("PUT conflict -> Append\n");
      isAppend = true;
    } else {
      currUser.writeLog("PUT conflict -> Overwrite\n");
    }
  } else {
    // Send FOUND
    newSend(socket_fd, "NOTFOUND", STATUS_SIZE);
  }
  currUser.writeLog("PUT mode: %s\n", (isBinary ? "Binary" : "Character"));
  if (isBinary)
    fptr = fopen(filePathC, "wb");
  else if (isAppend)
    fptr = fopen(filePathC, "a");
  else
    fptr = fopen(filePathC, "w");

  if (fptr == NULL) throw "Unable to create the file";

  // receive the file size
  size_t filesize;
  char recvBuffer[BUFFER_SIZE];
  newRecv(socket_fd, &filesize, sizeof(size_t));
  currUser.writeLog("File size %lu bytes\n", filesize);

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
  }
  currUser.writeLog("File received successfully\n");

  // close the fd
  fclose(fptr);
}

void handleGET(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
               bool isBinary) {
  char filename[CMD_ARG_SIZE];
  // read the file name and get the file path
  if (sscanf(command, "%*s %s", filename) != 1) {
    throw "Invalid filename";
  }
  std::string filePath = ("./Server/" + currUser.getUserName() + "/" +
                          std::string(sanitizeFileName(filename)));
  currUser.writeLog("GET %s\n", filePath.c_str());
  // try opening the file
  FILE* fptr;
  if (isBinary)
    fptr = fopen(filePath.c_str(), "rb");
  else
    fptr = fopen(filePath.c_str(), "r");

  if (fptr == NULL) {
    newSend(socket_fd, "NOTFOUND", STATUS_SIZE);
    currUser.writeLog("File not found\n");
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
    if (numBytes < 0) {
      fclose(fptr);
      throw "fread failed";
    }
    newSend(socket_fd, sendBuffer, numBytes);
    totBytesSent += numBytes;
  }
  currUser.writeLog("File sent successfully\n");
  fclose(fptr);
}

void handleMODE(int socket_fd, char command[CMD_SIZE], UserLogger& currUser,
                bool& isBinary) {
  char modeOption[STATUS_SIZE];
  if (sscanf(command, "%*s %s", modeOption) != 1) {
    throw "Invalid mode option";
  }
  if (strcasecmp(modeOption, "BIN") == 0) {
    currUser.writeLog("Set mode: Binary\n");
    isBinary = true;
  } else if (strcasecmp(modeOption, "CHAR") == 0) {
    currUser.writeLog("Set mode: Character\n");
    isBinary = false;
  } else {
    throw "Invalid mode option";
  }
}

void handleDEL(int socket_fd, char command[CMD_SIZE], UserLogger& currUser) {
  char filename[CMD_ARG_SIZE];
  // read the file name and get the file path
  if (sscanf(command, "%*s %s", filename) != 1) {
    throw "Invalid filename";
  }
  std::string filePath = ("./Server/" + currUser.getUserName() + "/" +
                          std::string(sanitizeFileName(filename)));
  const char* filePathC = filePath.c_str();
  currUser.writeLog("DELETE %s\n", filename);

  bool isFilePresent = (access(filePathC, F_OK) == 0);
  if (isFilePresent) {
    newSend(socket_fd, "FOUND", STATUS_SIZE);
  } else {
    newSend(socket_fd, "NOTFOUND", STATUS_SIZE);
    currUser.writeLog("File %s does not exist\n", filePathC);
    currUser.writeLog("Aborted delete\n");
    return;
  }
  char status[STATUS_SIZE];
  newRecv(socket_fd, status, STATUS_SIZE);
  if (strcasecmp(status, "DEL_ACK") != 0) {
    currUser.writeLog("Client aborted delete");
    return;
  }
  if (remove(filePathC) == 0) {
    newSend(socket_fd, "SUCCESS", STATUS_SIZE);
    currUser.writeLog("%s deleted successfully\n", filePathC);
  } else {
    newSend(socket_fd, "FAILED", STATUS_SIZE);
  }
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

std::string sanitizeFileName(std::string filePath) {
  const size_t last_slash_idx = filePath.find_last_of("/");
  if (std::string::npos != last_slash_idx) {
    filePath.erase(0, last_slash_idx + 1);
  }
  return filePath;
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
