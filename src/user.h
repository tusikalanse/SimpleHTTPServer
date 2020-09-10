#ifndef USER_H
#define USER_H

//the user class
class user {
 public:
  user(const char* _username, const char* _password);
  int getuserid();
  int joinroom(int _roomid);
  int exitroom();
  int login(const char* _password);
 private:
  static int usercount;
  int userid;
  char username[128];
  char password[128];
  int roomid;
};

#endif /* USER_H */
