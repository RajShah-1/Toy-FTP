#include "./include/Authorization.hpp"

Authorization::Authorization() {
  this->isLoggedIn = false;
  UserInfo user1("client_1", "pass1");
  UserInfo user2("client_2", "pass2");
  UserInfo user3("client_3", "pass3");
  UserInfo user4("client_4", "pass4");
  this->users.insert(std::make_pair(user1.name, user1));
  this->users.insert(std::make_pair(user2.name, user2));
  this->users.insert(std::make_pair(user3.name, user3));
  this->users.insert(std::make_pair(user4.name, user4));
}

bool Authorization::authenticate(std::string userName, std::string password) {
  UserInfo user = this->users[userName];
  return (password == user.password) {
    this->isLoggedIn = true;
    this->currentUser = &user;
    return true;
  }
  return false;
}

void Authorization::logout(void) {
  if (this->isLoggedIn) {
    this->currentUser = NULL;
  }
  this->isLoggedIn = false;
}

std::string Authorization::getCurrentUserName(void) {
  return this->currentUser.name;
}

bool Authorization::isActive(std::string userName) { return this->isLoggedIn; }