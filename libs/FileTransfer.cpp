#include "./include/FileTransfer.hpp"

FileTransfer::FileTransfer(int socket_fd) : socket_fd(socket_fd){};

void FileTransfer::sendFile(FILE* fptr, std::string fileName) {
  filesize_t fileSize = getFileSize(fptr);

  error_guard(send(socket_fd, fileName.c_str(), FILENAME_SIZE, 0) == -1,
              "sendFile::send failed");
  error_guard(send(socket_fd, &fileSize, sizeof fileSize, 0) == -1,
              "sendFile::send failed");

  int numbytes;
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  while ((numbytes = fread(buffer, sizeof(char), BUFFER_SIZE, fptr)) > 0) {
    error_guard(send(socket_fd, buffer, numbytes, 0) == -1,
                "sendFile::Send failed");
  }
  // fclose(fptr);
  printf("sendFile::Successful\n");
}

void FileTransfer::recvFile(std::string destFolder) {
  if (destFolder.length() > 0) {
    destFolder = destFolder + "/";
  }
  char recv_buffer[BUFFER_SIZE];
  int numbytes;
  char fileName[FILENAME_SIZE];
  filesize_t fileSize = -1;

  error_guard((numbytes = recv(socket_fd, fileName, FILENAME_SIZE, 0)) == -1,
              "recv_file::socket receive failed");
  printf("recv_file::Receiving: %s\n", fileName);

  error_guard((numbytes = recv(socket_fd, &fileSize, sizeof fileSize, 0)) == -1,
              "recv_file::socket receive failed");
  error_guard(fileSize <= 0, "recv_file::fileSize <= 0");

  std::string filePathStr = destFolder + fileName;
  FILE* fptr = fopen(filePathStr.c_str(), "wb");
  printf("FilePath: %s\n", filePathStr.c_str());
  error_guard(fptr == NULL, "recv_file::Unable to open/create the file");
  filesize_t totNumBytes = 0;
  while ((numbytes =
              recv(socket_fd, recv_buffer,
                   std::min(fileSize - totNumBytes, (unsigned long)BUFFER_SIZE),
                   0)) > 0) {
    error_guard(fwrite(recv_buffer, sizeof(char), numbytes, fptr) != numbytes,
                "recv_file::Error while writing the file");
    totNumBytes += numbytes;
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