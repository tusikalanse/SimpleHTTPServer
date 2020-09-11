#ifndef ROOM_HANDLER_H
#define ROOM_HANDLER_H

#include "mymemorypool.h"
#include "room.h"

#include <map>

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
class roomhandler {
 public:
  roomhandler();
  int init(int useripckey, int roomipckey);
  int registerUser(const char* username, const char* password);
  int createRoom(int userid, const char* roomname);
  int joinRoom(int userid, int roomid);
  int exitRoom(int userid);
 private:
  int USER_IPCKEY;
  int ROOM_IPCKEY;
  int roomsize;
  std::map<int, room*> roomlist;
  std::map<int, user*> userlist;
  MyMemoryPool<sizeof(user)> userpool;
  MyMemoryPool<sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user*)> roompool;
};

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::roomhandler() {
  USER_IPCKEY = ROOM_IPCKEY = -1;
}


template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::init(int useripckey, int roomipckey) {
  USER_IPCKEY = useripckey;
  userpool.init(useripckey, MAX_USER_COUNT_PER_ROOM * MAX_ROOM_COUNT);
  roompool.init(roomipckey, MAX_ROOM_COUNT);
  roomsize = sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user*);
}


#endif /* ROOM_HANDLER_H */
