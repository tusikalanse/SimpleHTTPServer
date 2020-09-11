#include "user.h"

#include <cstring>

int user::usercount = 0;

user::user(const char* _username, const char* _password) {
  strcpy(username, _username);
  strcpy(password, _password);
  userid = ++usercount;
  roomid = 0;
}

int user::getuserid() {
  return userid;
}

int user::joinroom(int _roomid) {
  if (roomid != 0) return -1;
  roomid = _roomid;
  return 0;
}

int user::exitroom() {
  if (roomid == 0) return -1;
  roomid = 0;
  return 0;
}

int user::login(const char* _password) {
  return strcmp(password, _password) == 0;
}
