#ifndef ROOM_HANDLER_H
#define ROOM_HANDLER_H

#include "mymemorypool.h"
#include "room.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

//用于处理房间和用户的关系
//并管理room和user的共享内存池
template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM> class roomhandler {
public:
  //构造函数，将USER_IPCKEY和ROOM_IPCKEY置为-1
  roomhandler();

  //初始化，根据给定参数申请共享内存池
  int init(int useripckey, int roomipckey);

  //注册新用户
  //用户名重复返回-1
  //用户数量已达上限返回-2
  //用户名或密码过长返回-3
  //注册成功返回0
  int registerUser(const char *username, const char *password);

  //用给定用户名和密码尝试登陆
  //成功登陆返回1
  int login(const char *username, const char *password);

  //获取用户所在房间id
  //用户不存在则返回-1，不在任何一个房间中返回0
  int getRoom(const char *username);

  //根据房间id获取房间名
  //房间不存在返回NULL
  const char *getRoomName(int roomid);

  //根据用户名获取id
  //用户不存在返回-1
  int getUserID(const char *username);

  //创建新房间
  //房间数量已达上限返回-1
  //房间名过长返回-2
  //成功则返回房间id
  int createRoom(const char *roomname);

  //用户加入房间
  //房间不存在返回-1
  //用户不存在返回-2
  //正在游戏中返回-3
  //用户已在某个房间中返回-4
  //加入执行失败返回-5
  //成功返回0
  int joinRoom(int userid, int roomid);

  //用户退出房间
  //用户不存在返回-1
  //用户不在房间中返回-2
  //成功返回0
  int exitRoom(int userid);

  //清空房间
  //返回清除的玩家数量
  //房间不存在返回-1
  int clearRoom(int roomid);


  //获取房间列表
  const std::vector<room *> &getRoomList();
  
  //获取房间列表
  std::map<int, room *> &getRoomMap();


private:
  //用户共享内存池key
  int USER_IPCKEY;

  //房间共享内存池key
  int ROOM_IPCKEY;

  //每个房间在共享内存中占用大小
  int roomsize;

  // map存储房间列表(id->房间地址)
  std::map<int, room *> roomlist;

  // vector同步存储房间列表，便于遍历
  std::vector<room *> roomVector;

  // map存储用户列表(id->用户地址)
  std::map<int, user *> userlist;

  // map存储用户列表(用户名->用户地址)
  std::map<std::string, user *> usernamelist;

  //用户共享内存池
  MyMemoryPool<sizeof(user)> userpool;

  //房间共享内存池
  MyMemoryPool<sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user *)>
      roompool;
};

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::roomhandler() {
  USER_IPCKEY = ROOM_IPCKEY = -1;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::init(int useripckey,
                                                               int roomipckey) {
  if (-1 == userpool.init(useripckey, MAX_USER_COUNT_PER_ROOM * MAX_ROOM_COUNT))
    return -1;
  if (-1 == roompool.init(roomipckey, MAX_ROOM_COUNT))
    return -1;
  USER_IPCKEY = useripckey;
  ROOM_IPCKEY = roomipckey;

  //将共享内存中的信息读取到对应map中
  user::usercount = userpool.size();
  for (int i = 0; i < MAX_USER_COUNT_PER_ROOM * MAX_ROOM_COUNT; ++i) {
    if (userpool.getstatus(i)) {
      user *tempUser = reinterpret_cast<user *>(userpool.get(i));
      userlist[tempUser->userid] = tempUser;
      usernamelist[std::string(tempUser->username)] = tempUser;
    }
  }
  room::roomcount = roompool.size();
  room::maxusercount = MAX_USER_COUNT_PER_ROOM;
  for (int i = 0; i < MAX_ROOM_COUNT; ++i) {
    if (roompool.getstatus(i)) {
      room *tempRoom = reinterpret_cast<room *>(roompool.get(i));
      roomlist[tempRoom->roomid] = tempRoom;
      roomVector.push_back(tempRoom);
    }
  }
  roomsize = sizeof(room) + MAX_USER_COUNT_PER_ROOM * sizeof(user *);
  return 0;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::registerUser(
    const char *username, const char *password) {
  if (usernamelist.count(std::string(username)))
    return -1;
  if (userpool.full())
    return -2;
  if (strlen(username) > 127 || strlen(password) > 127)
    return -3;
  user *newUser = reinterpret_cast<user *>(userpool.apply());
  new (newUser) user(username, password);
  userlist[newUser->userid] = newUser;
  usernamelist[std::string(newUser->username)] = newUser;
  return newUser->userid;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::login(
    const char *username, const char *password) {
  if (usernamelist.count(std::string(username)) == 0)
    return 0;
  return usernamelist[std::string(username)]->login(password);
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoom(
    const char *username) {
  if (usernamelist.count(std::string(username)) == 0)
    return -1;
  return usernamelist[std::string(username)]->getroom();
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
const char *
roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoomName(int roomid) {
  if (roomlist.find(roomid) == roomlist.end())
    return NULL;
  return roomlist[roomid]->roomname;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getUserID(
    const char *username) {
  if (usernamelist.count(std::string(username)) == 0)
    return -1;
  return usernamelist[std::string(username)]->getuserid();
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::createRoom(
    const char *roomname) {
  if (roompool.full())
    return -1;
  if (strlen(roomname) > 127)
    return -2;
  room *newRoom = reinterpret_cast<room *>(roompool.apply());
  new (newRoom) room(roomname);
  roomlist[newRoom->roomid] = newRoom;
  roomVector.push_back(newRoom);
  return newRoom->roomid;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::joinRoom(int userid,
                                                                   int roomid) {
  if (roomlist.count(roomid) == 0)
    return -1;
  if (userlist.count(userid) == 0)
    return -2;
  if (roomlist[roomid]->full() || roomlist[roomid]->gamestate == 1)
    return -3;
  if (userlist[userid]->getroom() != 0)
    return -4;
  if (userlist[userid]->joinroom(roomid) == -1)
    return -5;
  roomlist[roomid]->adduser(userlist[userid]);
  return 0;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::exitRoom(int userid) {
  if (userlist.count(userid) == 0)
    return -1;
  int roomid = userlist[userid]->getroom();
  if (roomid == 0)
    return -2;
  roomlist[roomid]->removeuser(userid);
  userlist[userid]->exitroom();
  return 0;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
int roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::clearRoom(int roomid) {
  if (roomlist.count(roomid) == 0)
    return -1;
  std::vector<int> ret = roomlist[roomid]->clear();
  roomlist[roomid]->gamestate = 0;
  for (int userid: ret) {
    userlist[userid]->exitroom();
  }
  return ret.size();
}


template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
const std::vector<room *> &
roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoomList() {
  return roomVector;
}

template <int MAX_ROOM_COUNT, int MAX_USER_COUNT_PER_ROOM>
std::map<int, room *>& roomhandler<MAX_ROOM_COUNT, MAX_USER_COUNT_PER_ROOM>::getRoomMap() {
  return roomlist;
}


#endif /* ROOM_HANDLER_H */
