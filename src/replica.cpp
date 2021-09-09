
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

int main(int ) {
  signal(SIGPIPE, SIG_IGN);

  int server_sockfd;
  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket error");
    return;
  }

  int opt = 1;
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt,
             sizeof(opt));

  if (bind(server_sockfd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
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

  while (1) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, 1000);
    if (-1 == nfds) {
      perror("epoll_wait fail");
      exit(EXIT_FAILURE);
    }
    timeout_queue.tick(time(NULL));
    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == server_sockfd) {
        if (!(events[n].events & EPOLLIN))
          continue;
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
      } else if (events[n].events & EPOLLIN) {
        int client_sockfd = events[n].data.fd;
        std::thread reader(&myserver::work, this, client_sockfd);
        reader.join();
      }
    }
  }
}


  static char buf[1024];
  while (true) {
    int ret = recv(client_sockfd, buf, sizeof(buf), 0);
    if (ret < 0) {
      if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {
        return;
      }
      close(client_sockfd);
      return;
    } else if (ret == 0) {
      close(client_sockfd);
      return;
    } else {
      buf[ret] = '\0';
      HTTPParser(client_sockfd, buf);
    }
  return 0;
}