#ifndef MY_SERVER_H
#define MY_SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "roomhandler.h"

//服务处理类
//用于处理相关网络服务
class myserver {
public:
  //默认用8000端口
  myserver();

  //用给定端口
  myserver(in_port_t _port);

  //给定用户和房间的共享内存号，开启服务
  void run(int USER_IPCKEY, int ROOM_IPCKEY);

private:
  //工作线程，处理新消息
  void work(int client_sockfd);

  //将接收到的消息划分成HTTP请求
  void HTTPParser(int client_sockfd, const char *buf);

  //处理get请求
  void dealGet(int client_sockfd, const char *buf, int len);

  //处理post请求
  void dealPost(int client_sockfd, const char *buf, const char *body, int len);

  //发送指定HTML页面
  void sendHTMLPage(int client_sockfd, const char *address);

  //指定提示和重定向网址发送成功页面
  void sendSuccessPage(int client_sockfd, const char *hint,
                       const char *redirect);

  //指定提示用户名密码和重定向网址发送成功页面
  void sendSuccessPage(int client_sockfd, const char *hint, const char *name,
                       const char *password, const char *redirect);

  //指定提示和重定向网址发送失败页面
  void sendErrorPage(int client_sockfd, const char *hint, const char *redirect);

  //指定提示用户名密码和重定向网址发送失败页面
  void sendErrorPage(int client_sockfd, const char *hint, const char *name,
                     const char *password, const char *redirect);

  //发送房间列表信息
  void sendRoomList(int client_sockfd, const char *name, const char *password);

  //发送房间信息
  void sendRoom(int client_sockfd, const char *name, const char *password,
                const char *roomname);

  //设置为非阻塞
  static void setnonblocking(int sockfd);

  //端口号
  in_port_t port;

  //最大连接数
  static const int MAX_EVENTS = 1024;

  //路径最大值
  static const int PATH_MAX = 512;

  //最大缓冲区
  static const int BUFFER_SIZE = 8192;

  // handler类，用于处理房间和用户的关系
  roomhandler<20, 5> handler;
};

#endif /* MY_SERVER_H */
