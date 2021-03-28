#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hpp"
const char* authFile = "./Server/auth.txt";

void removeUser(void);
void createUser(void);
bool isUserPresent(char enteredUserName[USERNAME_LEN]);

int main() {
  char command[CMD_SIZE];
  while (1) {
    printf("Enter [Create|Remove] (Not case sensitive): ");
    scanf(" %s", command);
    if (strcasecmp(command, "CREATE") == 0) {
      createUser();
    } else if (strcasecmp(command, "REMOVE") == 0) {
      removeUser();
    } else {
      printf("Invalid option\n");
    }
  }
  return 0;
}

void removeUser(void) {
  FILE* authFptrOrg = fopen(authFile, "r");
  FILE* authFptrDup = fopen("tmp.txt", "w+");
  if (authFptrOrg == NULL) {
    printf("Unable to open auth.txt");
    exit(EXIT_FAILURE);
  }
  if (authFptrDup == NULL) {
    printf("Unable to open tmp.txt");
    exit(EXIT_FAILURE);
  }

  char enteredUserName[USERNAME_LEN];
  printf("Enter username: ");
  scanf(" %s", enteredUserName);

  char username[USERNAME_LEN];
  char password[PASSWORD_LEN];
  // Copy all the usernames to authUsrDup except the one that is to be deleted
  while (fscanf(authFptrOrg, " %s %s", username, password) == 2) {
    if (strcmp(username, enteredUserName) != 0) {
      printf("%s %s\n", username, password);
      fprintf(authFptrDup, "%s %s\n", username, password);
    } else {
      // remove the directory of the deleted user
      char rmCmdDir[20 + 15 + USERNAME_LEN] = "exec rm -r ./Server/";
      char rmCmdLog[20 + 15 + USERNAME_LEN] = "exec rm -r ./Server/Logs/";
      strcat(rmCmdDir, username);
      strcat(rmCmdLog, username);
      printf("Executing %s\n", rmCmdDir);
      system(rmCmdDir);
      printf("Executing %s\n", rmCmdLog);
      system(rmCmdLog);
    }
  }
  rewind(authFptrDup);
  authFptrOrg = fopen(authFile, "w");
  
  // Copy authUsrDup to authUsrOrg
  while (fscanf(authFptrDup, " %s %s", username, password) == 2) {
    fprintf(authFptrOrg, "%s %s\n", username, password);
  }
  fclose(authFptrOrg);
  fclose(authFptrDup);
  remove("tmp.txt");
}

void createUser(void) {
  char username[USERNAME_LEN];
  char password[PASSWORD_LEN];
  printf("Enter username: ");
  scanf(" %s", username);
  if (isUserPresent(username)) {
    printf("User already exists\n");
    return;
  }
  printf("Enter password: ");
  scanf(" %s", password);

  FILE* authFptr = fopen(authFile, "a");
  if (authFptr == NULL) {
    printf("Error while opening the auth file\n");
    return;
  }
  // create a directory for that user
  char dirPath[15 + USERNAME_LEN] = "./Server/";
  char logPath[20 + USERNAME_LEN] = "./Server/Logs/";
  strcat(dirPath, username);
  strcat(logPath, username);
  if (mkdir(dirPath, 0777) != 0 || mkdir(logPath, 0777) != 0) {
    printf("Error while creating the user directory\n");
    printf("\tUsername should not contain / \n");
    printf("\tUsername can't be \"Logs\"\n");
    fclose(authFptr);
    return;
  }
  fprintf(authFptr, "%s %s\n", username, password);
  fclose(authFptr);
}

bool isUserPresent(char enteredUserName[USERNAME_LEN]) {
  FILE* authFptr = fopen(authFile, "r");
  if (authFptr == NULL) {
    printf("Unable to open auth.txt");
    exit(EXIT_FAILURE);
  }
  char username[USERNAME_LEN];
  char password[PASSWORD_LEN];
  while (fscanf(authFptr, " %s %s", username, password) == 2) {
    if (strcmp(username, enteredUserName) == 0) {
      fclose(authFptr);
      return true;
    }
  }
  return false;
}