#ifndef MY_SERVER_H
#define MY_SERVER_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
  static void setnonblocking(int sockfd);
  in_port_t port;
  static const int MAX_EVENTS = 1024;
};

#endif /* MY_SERVER_H */
