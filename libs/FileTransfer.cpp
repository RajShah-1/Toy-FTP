#include "./include/FileTransfer.hpp"

FileTransfer::FileTransfer(int socket_fd, bool printProgress, bool isClient)
    : socket_fd(socket_fd),
      isBinary(true),
      printProgress(printProgress),
      isClient(isClient){};

void FileTransfer::sendFile(FILE* fptr, std::string fileName) {
  filesize_t fileSize = getFileSize(fptr);
  int numbytes;
  char status[STATUS_SIZE];

  error_guard(send(socket_fd, fileName.c_str(), FILENAME_SIZE, 0) == -1,
              "sendFile::send failed");
  error_guard(send(socket_fd, &fileSize, sizeof fileSize, 0) == -1,
              "sendFile::send failed");

  error_guard((numbytes = recv(socket_fd, status, STATUS_SIZE, 0)) == -1,
              "recv_file::socket receive failed");
  if (strcasecmp(status, "READY") != 0) {
    printf("Unable to send the file...\n");
    return;
  }
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  int totNumBlocks = ceil((fileSize) / (double)BUFFER_SIZE);
  int sentBlocks = 0;
  while ((numbytes = fread(buffer, sizeof(char), BUFFER_SIZE, fptr)) > 0) {
    error_guard(send(socket_fd, buffer, numbytes, 0) == -1,
                "sendFile::Send failed");
    sentBlocks++;
    if (this->printProgress) {
      this->print_progress(sentBlocks, totNumBlocks);
    }
  }
  // fclose(fptr);
  printf("sendFile::Successful\n");
}

void FileTransfer::recvFile(std::string destFolder) {
  if (destFolder.length() > 0) {
    destFolder = destFolder + "/";
  }
  char recv_buffer[BUFFER_SIZE];
  // char path_buffer[PATH_MAX];
  int numbytes;
  char sentFilePath[FILENAME_SIZE];
  filesize_t fileSize = -1;

  error_guard(
      (numbytes = recv(socket_fd, sentFilePath, FILENAME_SIZE, 0)) == -1,
      "recv_file::socket receive failed");

  // Sanitizes the sent file path
  std::string fileName = this->getFileName(sentFilePath);
  printf("recv_file::Receiving: %s\n", fileName.c_str());
  std::string filePathStr = destFolder + fileName;
  printf("FilePath: %s\n", filePathStr.c_str());

  FILE* fptr;

  if (this->isBinary) {
    fptr = fopen(filePathStr.c_str(), "wb");
  } else {
    fptr = fopen(filePathStr.c_str(), "w");
  }
  if (fptr == NULL) {
    // send fail to user
    printf("%s\n", filePathStr.c_str());
    error_guard(send(socket_fd, "ERROR", STATUS_SIZE, 0) == -1, "send failed");
    return;
  }
  error_guard(send(socket_fd, "READY", STATUS_SIZE, 0) == -1, "send failed");

  error_guard((numbytes = recv(socket_fd, &fileSize, sizeof fileSize, 0)) == -1,
              "recv_file::socket receive failed");
  error_guard(fileSize <= 0, "recv_file::fileSize <= 0");

  filesize_t totNumBytes = 0;
  int totNumBlocks = ceil((fileSize) / (double)BUFFER_SIZE);
  int receivedBlocks = 0;
  while ((numbytes =
              recv(socket_fd, recv_buffer,
                   std::min(fileSize - totNumBytes, (unsigned long)BUFFER_SIZE),
                   0)) > 0) {
    error_guard(fwrite(recv_buffer, sizeof(char), numbytes, fptr) != numbytes,
                "recv_file::Error while writing the file");
    totNumBytes += numbytes;
    receivedBlocks++;
    if (this->printProgress) {
      this->print_progress(receivedBlocks, totNumBlocks);
    }
    if (totNumBytes >= fileSize) {
      break;
    }
    memset(recv_buffer, 0, BUFFER_SIZE);
    // printf("recv_file::Wrote %lu bytes\n", totNumBytes);
  }
  fclose(fptr);
  printf("recv_file::File received successfully: Wrote %lu bytes\n",
         totNumBytes);
  error_guard(numbytes == -1, "recv_file::Receive failed");
}

filesize_t FileTransfer::getFileSize(FILE* fptr) {
  filesize_t fileSize;
  struct stat fileInfo;
  // go to the end of the file
  error_guard(fseek(fptr, 0L, SEEK_END) != 0, "getFileSize::fseek failed");
  // read the current position (gives fileSize)
  fileSize = ftell(fptr);
  error_guard(fileSize < 0, "getFileSize::ftell failed");
  // go back to the start of the file
  rewind(fptr);
  printf("getFileSize::File Size: %lu\n", fileSize);
  return fileSize;
}

void FileTransfer::setMode(const char mode[1]) {
  if (strcasecmp(mode, "B") == 0) {
    this->isBinary = true;
    if (this->isClient) {
      error_guard(send(socket_fd, "MODEB", COMMAND_SIZE, 0) == -1,
                  "send failed");
    }
    printf("MODE:B\n");
  } else if (strcasecmp(mode, "C") == 0) {
    this->isBinary = false;
    if (this->isClient) {
      error_guard(send(socket_fd, "MODEC", COMMAND_SIZE, 0) == -1,
                  "send failed");
    }
    printf("MODE:C\n");
  } else {
    printf("Entered invalid mode option\n");
  }
}

const char* FileTransfer::getMode(void) { return this->isBinary ? "B" : "C"; }

std::string FileTransfer::getFileName(std::string filePath) {
  const size_t last_slash_idx = filePath.find_last_of("/");
  if (std::string::npos != last_slash_idx) {
    filePath.erase(0, last_slash_idx + 1);
  }
  return filePath;
}

// References:
// https://stackoverflow.com/questions/1508490/erase-the-current-printed-console-line/1508589#1508589
// https://stackoverflow.com/questions/338273/why-does-printf-not-print-anything-before-sleep/338295#338295
void FileTransfer::print_progress(unsigned int eFinished, unsigned int eTotal) {
  unsigned int finished = (int)(PROMPT_SIZE * eFinished / (double)eTotal);
  unsigned int total = PROMPT_SIZE;
  char buffer[PROMPT_SIZE + 5];
  buffer[0] = '[';
  for (unsigned int i = 1; i <= total; ++i) {
    if (i <= finished - 1) {
      buffer[i] = '=';
    } else if (i == finished) {
      buffer[i] = '>';
    } else {
      buffer[i] = ' ';
    }
  }
  buffer[PROMPT_SIZE + 1] = ']';
  buffer[PROMPT_SIZE + 2] = '\0';
  printf("\r%s", buffer);
  fflush(stdout);
}