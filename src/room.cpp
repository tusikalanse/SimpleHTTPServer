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
}

int room::full() { return usercount == maxusercount; }

int room::count() { return usercount; }
