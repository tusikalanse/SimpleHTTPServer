#include "room.h"

#include <cstring>

int room::roomcount = 0;
int room::maxusercount = 0;

room::room(const char *_roomname) {
  strcpy(roomname, _roomname);
  roomid = ++roomcount;
  for (int i = 0; i < maxusercount; ++i) {
    userlist[i] = NULL;
  }
  gamestate = 0;
  usercount = 0;
}

int room::removeuser(int userid) {
  for (int i = 0; i < maxusercount; ++i) {
    if (userlist[i] != NULL) {
      if (userlist[i]->getuserid() == userid) {
        userlist[i] = NULL;
        usercount--;
        return 0;
      }
    }
  }
  return -1;
}

int room::adduser(user *newuser) {
  if (usercount == maxusercount)
    return -1;
  for (int i = 0; i < maxusercount; ++i) {
    if (userlist[i] == NULL) {
      userlist[i] = newuser;
      usercount++;
      return 0;
    }
  }
  return -1;
}

std::vector<int> room::clear() {
  std::vector<int> ret;
  for (int i = 0; i < maxusercount; ++i) {
    if (userlist[i] != NULL) {
      ret.push_back(userlist[i]->getuserid());
      usercount--;
      userlist[i] = NULL;
    }
  }
  return ret;
}


int room::full() { return usercount == maxusercount; }

int room::count() { return usercount; }
