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
  int server();
 private:
  int 
};


#endif /* MY_SERVER_H */
