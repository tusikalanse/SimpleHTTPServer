#ifndef ROOM_HANDLER_H
#define ROOM_HANDLER_H

#include "mymemorypool.h"
#include "room.h"

#include <cstring>
#include <string>
#include <map>
#include <vector>

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
class roomhandler {
 public:
  roomhandler();
  int init(int useripckey, int roomipckey);
  int registerUser(const char* username, const char* password);
  int login(const char* username, const char* password);
  int getRoom(const char* username);
  const char* getRoomName(int roomid);
  int getUserID(const char* username);
  int createRoom(const char* roomname);
  int joinRoom(int userid, int roomid);
  int exitRoom(int userid);
  const std::vector<room*>& getRoomList();
 private:
  int USER_IPCKEY;
  int ROOM_IPCKEY;
  int roomsize;
  std::map<int, room*> roomlist;
  std::vector<room*> roomVector;
  std::map<int, user*> userlist;
  std::map<std::string, user*> usernamelist;
  MyMemoryPool<sizeof(user)> userpool;
  MyMemoryPool<sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user*)> roompool;
};

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::roomhandler() {
  USER_IPCKEY = ROOM_IPCKEY = -1;
}


template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::init(int useripckey, int roomipckey) {
  if (-1 == userpool.init(useripckey, MAX_USER_COUNT_PER_ROOM * MAX_ROOM_COUNT))
    return -1;
  if (-1 == roompool.init(roomipckey, MAX_ROOM_COUNT))
    return -1;
  USER_IPCKEY = useripckey;
  ROOM_IPCKEY = roomipckey;
  user::usercount = userpool.size();
  for (int i = 0; i < MAX_USER_COUNT_PER_ROOM * MAX_ROOM_COUNT; ++i) {
    if (userpool.getstatus(i)) {
      user* tempUser = reinterpret_cast<user*>(userpool.get(i));
      userlist[tempUser->userid] = tempUser;
      usernamelist[std::string(tempUser->username)] = tempUser;
    }
  }
  room::roomcount = roompool.size();
  room::maxusercount = MAX_USER_COUNT_PER_ROOM;
  for (int i = 0; i < MAX_ROOM_COUNT; ++i) {
    if (roompool.getstatus(i)) {
      room* tempRoom = reinterpret_cast<room*>(roompool.get(i));
      roomlist[tempRoom->roomid] = tempRoom;
      roomVector.push_back(tempRoom);
    }
  }
  roomsize = sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user*);
  return 0;
} 

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::registerUser(const char* username, const char* password) {
  if (usernamelist.count(std::string(username))) return -1;
  if (userpool.full()) return -2;
  if (strlen(username) > 127 || strlen(password) > 127) return -3;
  user* newUser = reinterpret_cast<user*>(userpool.apply());
  new (newUser) user(username, password);
  userlist[newUser->userid] = newUser;
  usernamelist[std::string(newUser->username)] = newUser;
  return newUser->userid;
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::login(const char* username, const char* password) {
  if (usernamelist.count(std::string(username)) == 0) return 0;
  return usernamelist[std::string(username)]->login(password);
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoom(const char* username) {
  if (usernamelist.count(std::string(username)) == 0) return -1;
  return usernamelist[std::string(username)]->getroom();
}


template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
const char* roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoomName(int roomid) {
  if (roomlist.find(roomid) == roomlist.end()) return NULL;
  return roomlist[roomid]->roomname;
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getUserID(const char* username) {
  if (usernamelist.count(std::string(username)) == 0) return -1;
  return usernamelist[std::string(username)]->getuserid();
}


template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::createRoom(const char* roomname) {
  if (roompool.full()) return -1;
  if (strlen(roomname) > 127) return -2;
  room* newRoom = reinterpret_cast<room*>(roompool.apply());
  new (newRoom) room(roomname);
  roomlist[newRoom->roomid] = newRoom;
  roomVector.push_back(newRoom);
  return newRoom->roomid;
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::joinRoom(int userid, int roomid) {
  if (roomlist.count(roomid) == 0) return -1;
  if (userlist.count(userid) == 0) return -2;
  if (roomlist[roomid]->full()) return -3;
  if (userlist[userid]->getroom() != 0) return -4;
  if (userlist[userid]->joinroom(roomid) == -1) return -5;
  roomlist[roomid]->adduser(userlist[userid]);
  return 0;
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::exitRoom(int userid) {
  if (userlist.count(userid) == 0) return -1;
  int roomid = userlist[userid]->getroom();
  if (roomid == 0) return -2;
  roomlist[roomid]->removeuser(userid);
  userlist[userid]->exitroom();
  return 0;
}

template<int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
const std::vector<room*>& roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoomList() {
  return roomVector;
}

#endif /* ROOM_HANDLER_H */
