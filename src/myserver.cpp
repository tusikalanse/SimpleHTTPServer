#include "myserver.h"

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

myserver::myserver() : 
    port(8000) 
  {}

myserver::myserver(in_port_t _port) : 
    port(_port) 
  {}

void myserver::run(int USER_IPCKEY, int ROOM_IPCKEY) {
  handler.init(USER_IPCKEY, ROOM_IPCKEY);

  int server_sockfd;
  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);
  
  if ((server_sockfd = socket(PF_INET,SOCK_STREAM,0)) < 0) {  
    perror("socket error");
    return;
  }

  int opt = 1;
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

  if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind error");
    return;
  }

  if (listen(server_sockfd, 128) < 0) {
    perror("listen error");
    return;
  }

  struct epoll_event ev, events[MAX_EVENTS];
  int epollfd = epoll_create1(0);
  if (-1 == epollfd) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }
  ev.events = EPOLLIN; 
  ev.data.fd = server_sockfd;
  if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, server_sockfd, &ev)) {
    perror("epoll_ctl EPOLL_CTL_ADD fail");
    exit(EXIT_FAILURE);
  }

  int client_sockfd;
  sockaddr_in client_addr;
  unsigned int sin_size = sizeof(client_addr);
  while (1) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (-1 == nfds) {
      perror("epoll_wait fail");
      exit(EXIT_FAILURE);
    }
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == server_sockfd) {
        if (!(events[n].events & EPOLLIN)) continue;
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(server_sockfd, (sockaddr *)&cliaddr, &len);
        if (-1 == connfd) {
          perror("accept fail");
          continue;
        }
        setnonblocking(connfd);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = connfd;
        if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev)) {
          perror("epoll_ctl EPOLL_CTL_ADD fail");
          close(connfd);
          continue;
        }
        char buff[INET_ADDRSTRLEN + 1] = {0};
        inet_ntop(AF_INET, &cliaddr.sin_addr, buff, INET_ADDRSTRLEN);
        uint16_t port = ntohs(cliaddr.sin_port);
        printf("connection from %s, port %d\n", buff, port);
      } 
      else if (events[n].events & EPOLLIN) {
        int client_sockfd = events[n].data.fd;
        std::thread reader(&myserver::work, this, client_sockfd);
        reader.join();
      }
    }
  }
}

void myserver::work(int client_sockfd) {
  static char buf[1024];
  while (true) {
    int ret = recv(client_sockfd, buf, sizeof(buf), 0);
    if (ret < 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
        return;
      }
      close(client_sockfd);
      return;
    }
    else if (ret == 0) {
      close(client_sockfd);
      return;
    }  
    else {
      buf[ret] = '\0';
      HTTPParser(client_sockfd, buf);
    }
  }
}

void myserver::HTTPParser(int client_sockfd, const char* buf) {
  int n = strlen(buf), IDX = 0;
  while (IDX < n) {
    if (buf[IDX] == 'G') {
      const char* temp = strstr(buf + IDX, "\r\n\r\n");
      dealGet(client_sockfd, buf + IDX, temp - buf - IDX + 4);
      IDX = temp - buf + 4;
    }
    else {
      const char* temp = strstr(buf + IDX, "Content-Length:");
      int length = 0;
      char ch = *temp;
      while (ch < '0' || ch > '9') ch = *++temp;
      while (ch >= '0' && ch <= '9') {
        length = length * 10 + ch - '0';
        ch = *++temp;
      }
      temp = strstr(temp, "\r\n\r\n");
      dealPost(client_sockfd, buf + IDX, temp + 4, length);
      IDX = temp - buf + length + 4;
    }
  }
}

void myserver::dealGet(int client_sockfd, const char* buf, int len) {
  const char* temp = strchr(buf, '/');
  if (strstr(temp, "register") == temp + 1) {
    //todo 发送register页面
    sendHTMLPage(client_sockfd, "register");
  }
  else if (strstr(temp, "login") == temp + 1) {
    sendHTMLPage(client_sockfd, "login");
  }
  else if (strstr(temp, "roomlist") == temp + 1) {
    if (temp[8] != '?') 
      sendHTMLPage(client_sockfd, "login");
    else {
      const char* name = strstr(buf, "name=");
      char* password = const_cast<char*>(strstr(buf, "&password="));
      password[0] = '\0';
      char* end = const_cast<char*>(strstr(buf, "\r\n"));
      end[0] = '\0';
      int success = handler.login(name + 5, password + 10);
      if (success) {
        int room = handler.getRoom(name + 5);
        if (room == 0) {
          sendRoomList(client_sockfd, name, password);
        }
        else {
          sendRoom(client_sockfd, name, password, handler.getRoomName(room));
        }
      }
      else 
        sendHTMLPage(client_sockfd, "login");
    }
  }
  else if (strstr(temp, "join") == temp + 1) {
    const char* name = strstr(buf, "name=");
    char* password = const_cast<char*>(strstr(buf, "&password="));
    password[0] = '\0';
    char* room = const_cast<char*>(strstr(buf, "&roomid="));
    int roomid = atoi(room + 8);
    int userid = handler.getUserID(name + 5);
    char* end = const_cast<char*>(strstr(buf, "\r\n"));
    end[0] = '\0';
    int loginStatus = handler.login(name + 5, password + 10);
    if (loginStatus == 0) {
      sendErrorPage(client_sockfd, "Wrong Username or Password", "login");
    }
    else {
      int joinStatus = handler.joinRoom(userid, roomid);
      if (joinStatus == -1) {
        sendErrorPage(client_sockfd, "Room does not exist", name + 5, password + 10, "roomlist");
      }
      else if (joinStatus == -2) {
        sendErrorPage(client_sockfd, "User does not exist", "login");
      }
      else if (joinStatus == -3) {
        sendErrorPage(client_sockfd, "The room is full", name + 5, password + 10, "roomlist");
      }
      else if (joinStatus == -4) {
        sendErrorPage(client_sockfd, "Please exit your room first", name + 5, password + 10, "roomlist");
      }
      else if (joinStatus == -5) {
        sendErrorPage(client_sockfd, "Join room failed",  name + 5, password + 10, "roomlist");
      }
      else if (joinStatus == 0) {
        sendRoom(client_sockfd, name + 5, password + 10, room + 8);
      }
    }
  }
  else {
    sendErrorPage(client_sockfd, "notfound", "login");
  }
}

void myserver::dealPost(int client_sockfd, const char* buf, const char* body, int len) {
  const char* temp = strchr(buf, '/');
  if (strstr(temp, "register") == temp + 1) {
    //todo 发送register页面
    const char* name = strstr(body, "name=");
    char* password = const_cast<char*>(strstr(body, "&password="));
    password[0] = '\0';
    char* end = const_cast<char*>(strstr(body, "\r\n"));
    end[0] = '\0';
    int registerStatus = handler.registerUser(name + 5, password + 10);
    if (registerStatus == -1) {
      sendErrorPage(client_sockfd, "User already exist", "register");
    }
    else if (registerStatus == 2) {
      sendErrorPage(client_sockfd, "Server refused your registration", "register");
    }
    else if (registerStatus == 3) {
      sendErrorPage(client_sockfd, "Username or password too long", "register");
    }
    else {
      sendSuccessPage(client_sockfd, "Successfully registered", "login");
    }
  }
  else if (strstr(temp, "login") == temp + 1) {
    const char* name = strstr(body, "name=");
    char* password = const_cast<char*>(strstr(body, "&password="));
    password[0] = '\0';
    char* end = const_cast<char*>(strstr(body, "\r\n"));
    end[0] = '\0';
    int loginStatus = handler.login(name + 5, password + 10);
    if (loginStatus == 0) {
      sendErrorPage(client_sockfd, "Wrong Username or Password", "login");
    }
    else {
      int room = handler.getRoom(name + 5);
      if (room == 0) {
        sendRoomList(client_sockfd, name + 5, password + 10);
      }
      else {
        sendRoom(client_sockfd, name + 5, password + 10, name + 5);
      }
    }
  }
  else if (strstr(temp, "exit") == temp + 1) {
    const char* name = strstr(body, "name=");
    char* password = const_cast<char*>(strstr(body, "&password="));
    password[0] = '\0';
    int userid = handler.getUserID(name + 5);
    char* end = const_cast<char*>(strstr(body, "\r\n"));
    end[0] = '\0';
    int loginStatus = handler.login(name + 5, password + 10);
    if (loginStatus == 0) {
      sendErrorPage(client_sockfd, "Wrong Username or Password", "login");
    }
    else {
      int exitStatus = handler.exitRoom(userid);
      if (exitStatus == -1) {
        sendErrorPage(client_sockfd, "User does not exist", "login");
      }
      else if (exitStatus == -2) {
        sendErrorPage(client_sockfd, "You and not in a room", name + 5, password + 10, "roomlist");
      }
      else if (exitStatus == 0) {
        sendRoomList(client_sockfd, name + 5, password + 10);
      }
    }
  }
  else if (strstr(temp, "create") == temp + 1) {
    const char* name = strstr(body, "name=");
    char* password = const_cast<char*>(strstr(body, "&password="));
    password[0] = '\0';
    char* room = const_cast<char*>(strstr(body, "&roomname="));
    int userid = handler.getUserID(name + 5);
    char* end = const_cast<char*>(strstr(body, "\r\n"));
    end[0] = '\0';
    int loginStatus = handler.login(name + 5, password + 10);
    if (loginStatus == 0) {
      sendErrorPage(client_sockfd, "Wrong Username or Password", "login");
    }
    else {
      int createStatus = handler.createRoom(room + 10);
      if (createStatus == -1) {
        sendErrorPage(client_sockfd, "The number of rooms has reached the limit", name + 5, password + 10, "roomlist");
      }
      else {
        sendSuccessPage(client_sockfd, "Create new room success!", name + 5, password + 10, "roomlist");
      }
    }
  }
  else {
    sendErrorPage(client_sockfd, "notfound", "login");
  }
}

void myserver::sendHTMLPage(int client_sockfd, const char* address) {
  char path[PATH_MAX];
  char buffer[BUFFER_SIZE];
  sprintf(path, "html/%s.html", address);
  struct stat st;
  int file_fd = open(path, O_RDONLY);
  off_t offset = 0;
  stat(path, &st);
  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Length: %d\r\n", st.st_size);
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Type: text/html\r\n\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sendfile(client_sockfd, file_fd, &offset, st.st_size);
}

void myserver::sendSuccessPage(int client_sockfd, const char* hint, const char* redirect) {
  char path[PATH_MAX];
  char buffer[BUFFER_SIZE];
  char file[BUFFER_SIZE];
  sprintf(path, "html/success.html");
  FILE* fp = fopen(path, "rb");
  fread(buffer, 1, BUFFER_SIZE, fp);
  sprintf(file, buffer, redirect, hint);
  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Length: %d\r\n", strlen(file));
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Type: text/html\r\n\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  send(client_sockfd, file, strlen(file), 0);
}

void myserver::sendSuccessPage(int client_sockfd, const char* hint, const char* name, const char* password, const char* redirect) {
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "%s?name=%s&password=%s", redirect, name, password);
  sendSuccessPage(client_sockfd, hint, buffer);
}

void myserver::sendErrorPage(int client_sockfd, const char* hint, const char* redirect) {
  char path[PATH_MAX];
  char buffer[BUFFER_SIZE];
  char file[BUFFER_SIZE];
  sprintf(path, "html/error.html");
  FILE* fp = fopen(path, "rb");
  fread(buffer, 1, BUFFER_SIZE, fp);
  sprintf(file, buffer, redirect, hint);
  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Length: %d\r\n", strlen(file));
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Type: text/html\r\n\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  send(client_sockfd, file, strlen(file), 0);
}

void myserver::sendErrorPage(int client_sockfd, const char* hint, const char* name, const char* password, const char* redirect) {
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "%s?name=%s&password=%s", redirect, name, password);
  sendErrorPage(client_sockfd, hint, buffer);
}

void myserver::sendRoomList(int client_sockfd, const char* name, const char* password) {
  char path[PATH_MAX];
  char buffer[BUFFER_SIZE];
  char file[BUFFER_SIZE];
  char roomlist[BUFFER_SIZE];
  roomlist[0] = 0;
  const std::vector<room*>& rooms = handler.getRoomList();
  for (int i = 0; i < rooms.size(); ++i) {
    int len = strlen(roomlist);
    sprintf(roomlist + len, "<div>\r\n<p><span style=\"text-decoration:none;\">%s</span><a>\
    <span style=\"text-decoration:none;color:#0000FF;\" onclick=\"location='join?name=%s&password=%s&roomid=%d'\">注册</span></a><span style=\"text-decoration:none;\">一个</span></p></div>", \
    rooms[i]->roomname, name, password, rooms[i]->roomid);
  }
  sprintf(path, "html/roomlist.html");
  FILE* fp = fopen(path, "rb");
  fread(buffer, 1, BUFFER_SIZE, fp);
  sprintf(file, buffer, name, password, roomlist);
  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Length: %d\r\n", strlen(file));
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Type: text/html\r\n\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  send(client_sockfd, file, strlen(file), 0);
}

void myserver::sendRoom(int client_sockfd, const char* name, const char* password, const char* roomname) {
  char path[PATH_MAX];
  char buffer[BUFFER_SIZE];
  char file[BUFFER_SIZE];
  sprintf(path, "html/room.html");
  FILE* fp = fopen(path, "rb");
  fread(buffer, 1, BUFFER_SIZE, fp);
  sprintf(file, buffer, roomname, name, password, roomname);
  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Length: %d\r\n", strlen(file));
  send(client_sockfd, buffer, strlen(buffer), 0);
  sprintf(buffer, "Content-Type: text/html\r\n\r\n");
  send(client_sockfd, buffer, strlen(buffer), 0);
  send(client_sockfd, file, strlen(file), 0);
}

void myserver::setnonblocking(int sockfd) {
  int flag = fcntl(sockfd, F_GETFL, 0);
  if (flag < 0) {
    perror("fcntl F_GETFL fail");
    return;
  }
  if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
    perror("fcntl F_SETFL fail");
  }
}
