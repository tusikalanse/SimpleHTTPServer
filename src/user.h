#ifndef USER_H
#define USER_H

//用户结构体
//保存用户id，用户名，密码，所在房间
struct user {
  //创建用户
  user(const char* _username, const char* _password);
  
  //获取用户id
  int getuserid();

  //获取用户所在房间id
  //若用户不在任何一个房间返回0，否则返回房间id
  int getroom();

  //加入房间
  //若已经处于一个房间中则返回-1表示失败
  //否则加入成功返回0
  int joinroom(int _roomid);

  //退出所在房间
  //若不在任意房间中返回-1，否则返回0
  int exitroom();

  //用指定密码登陆
  //登陆成功返回1
  int login(const char* _password);
  
  //用于表示下一个用户的id
  static int usercount;

  //当前用户id
  int userid;

  //用户名
  char username[128];

  //密码
  char password[128];
  
  //所在房间id
  int roomid;
};

#endif /* USER_H */
