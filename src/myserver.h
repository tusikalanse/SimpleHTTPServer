#ifndef MY_SERVER_H
#define MY_SERVER_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "roomhandler.h"

class myserver {
 public:
  myserver();
  myserver(in_port_t _port);
  void run(int USER_IPCKEY, int ROOM_IPCKEY);
 private:
  void work(int client_sockfd);
  void HTTPParser(int client_sockfd, const char* buf);
  void dealGet(int client_sockfd, const char* buf, int len);
  void dealPost(int client_sockfd, const char* buf, const char* body, int len);
  void sendHTMLPage(int client_sockfd, const char* address);
  void sendSuccessPage(int client_sockfd, const char* hint, const char* redirect);
  void sendSuccessPage(int client_sockfd, const char* hint, const char* name, const char* password, const char* redirect);
  void sendErrorPage(int client_sockfd, const char* hint, const char* redirect);
  void sendErrorPage(int client_sockfd, const char* hint, const char* name, const char* password, const char* redirect);
  void sendRoomList(int client_sockfd, const char* name, const char* password);
  void sendRoom(int client_sockfd, const char* name, const char* password, const char* roomname);
  static void setnonblocking(int sockfd);
  in_port_t port;
  static const int MAX_EVENTS = 1024;
  static const int PATH_MAX = 512;
  static const int BUFFER_SIZE = 8192;
  roomhandler<20, 5> handler;
};

#endif /* MY_SERVER_H */
