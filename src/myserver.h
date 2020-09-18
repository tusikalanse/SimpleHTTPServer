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
  void run();
 private:
  void work(int client_sockfd);
  void HTTPParser(int client_sockfd, const char* buf);
  void dealGet(int client_sockfd, const char* buf, int len);
  void dealPost(int client_sockfd, const char* buf, const char* body, int len);
  void sendHTMLPage(int client_sockfd, const char* address);
  void sendSuccessPage(int client_s, const char* hint);
  void sendSuccessPage(int client_s, const char* hint, const char* name, const char* password);
  void sendErrorPage(int client_s, const char* hint);
  void sendErrorPage(int client_s, const char* hint, const char* name, const char* password);
  void sendRoomList(int client_sockfd, const char* name, const char* password);
  void sendRoom(int client_sockfd, const char* name, const char* password, int roomid);
  static void setnonblocking(int sockfd);
  in_port_t port;
  static const int MAX_EVENTS = 1024;
  roomhandler<20, 5> handler;
};

#endif /* MY_SERVER_H */
