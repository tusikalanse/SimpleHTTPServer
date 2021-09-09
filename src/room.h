#ifndef ROOM_H
#define ROOM_H

#include "user.h"

#include <vector>

//房间结构体
//保存房间id，名称，当前用户数量，每个用户指针
struct room {
  //用给定名称创建房间
  room(const char *_roomname);

  //按照id查找并移除用户
  //成功移除返回0，否则返回-1(该用户不存在于当前房间)
  int removeuser(int userid);

  //房间添加用户
  //房间已满无法添加返回-1，成功添加返回0
  int adduser(user *newuser);

  //清空用户
  //返回清空的用户id vector
  std::vector<int> clear();

  //返回房间是否为空
  int full();

  //返回房间内用户数量
  int count();

  //房间最大数量
  static int roomcount;

  //房间最大用户数量
  static int maxusercount;

  //房间id
  int roomid;

  //房间名称
  char roomname[128];

  //当前房间用户数量
  int usercount;

  //开局状态, 0为等待, 1为游戏中
  int gamestate;

  //用户指针，
  user *userlist[0];
};

#endif /* ROOM_H */
