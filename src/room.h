#ifndef ROOM_H
#define ROOM_H

#include "user.h"

struct room {
  room(const char* _roomname);
  int removeuser(int userid);
  int adduser(user* newuser);
  int full();
  int count();
  static int roomcount;
  static int maxusercount;
  int roomid;
  char roomname[128];
  int usercount;
  user* userlist[0];
};

#endif /* ROOM_H */
