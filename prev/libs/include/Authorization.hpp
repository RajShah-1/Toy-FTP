#ifndef __AUTHORIZATION_H
#define __AUTHORIZATION_H

#include <string>
#include <unordered_map>
#include <utility>

#include "common.hpp"

const int USERNAME_SIZE = 10;
const int PASSWORD_SIZE = 10;

struct UserInfo {
  std::string name;
  std::string password;
  UserInfo(std::string name, std::string password)
      : name(name), password(password) {}
  UserInfo(){}
};

class Authorization {
  std::unordered_map<std::string, UserInfo> users;
  UserInfo* currentUser;
  bool isLoggedIn;

 public:
  Authorization();
  ~Authorization();
  bool authenticate(std::string userName, std::string password);
  void logout(void);
  std::string getCurrentUserName(void);
  bool isUserLoggedIn(std::string userName);
};

#endif