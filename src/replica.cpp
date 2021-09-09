
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

const int MAX_EVENTS = 1024;

inline void setnonblocking(int sockfd) {
  int flag = fcntl(sockfd, F_GETFL, 0);
  if (flag < 0) {
    perror("fcntl F_GETFL fail");
    return;
  }
  if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
    perror("fcntl F_SETFL fail");
  }
}

char main_buf[8192];
int main_len;

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "Usage: ./REPLICA port main_ip main_port" << std::endl;
    return 0;
  }
  signal(SIGPIPE, SIG_IGN);

  int port = atoi(argv[1]);
  std::string main_ip = std::string(argv[2]);
  int main_port = atoi(argv[3]);
  int main_sockfd;
  sockaddr_in main_addr;
  memset(&main_addr, 0, sizeof(main_addr));
  main_addr.sin_family = AF_INET;
  main_addr.sin_addr.s_addr = inet_addr(main_ip.c_str());
  main_addr.sin_port = htons(main_port);

  if ((main_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket error");
    return 0;
  }

	if(connect(main_sockfd, (struct sockaddr *)&main_addr, sizeof(struct sockaddr)) < 0){
		perror("connect error");
		return 1;
	}
	printf("connected to main server\n");

	// main_len = recv(main_sockfd, main_buf, 8192, 0);			//接收服务器端信息
  // main_buf[main_len] = '\0';
	// printf("%s", main_buf);									//打印服务器端的欢迎信息


  int server_sockfd;
  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket error");
    return 0;
  }


	// while(1) {
  //   printf("print message:\n");
  //   scanf("%s", main_buf);
  //   main_len = send(main_sockfd, main_buf, strlen(main_buf), 0);
  //   printf("send message end: %d\n", main_len);
    
  //   if((main_len = recv(main_sockfd, main_buf, 8192, 0)) > 0){
  //     main_buf[main_len] = '\0';
  //     printf("Recive from server: %s\n", main_buf);
  //   }
  //   usleep(200000);
  // }


  int opt = 1;
  setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt,
             sizeof(opt));

  if (bind(server_sockfd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("bind error");
    return 0;
  }

  if (listen(server_sockfd, 128) < 0) {
    perror("listen error");
    return 0;
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
        static char buf[8192];
        static int buf_pos = 0;
        while (true) {
          int ret = recv(client_sockfd, buf + buf_pos, sizeof(buf) - buf_pos, 0);
          if (ret < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) {

              break;
            }
            close(client_sockfd);
            break;
          } else if (ret == 0) {
            close(client_sockfd);
            break;
          } else {
            buf_pos += ret;
            buf[buf_pos] = '\0';

            main_len = send(main_sockfd, buf, strlen(buf), 0);
            // printf("send message end: %d\n", main_len);
            
            if((main_len = recv(main_sockfd, main_buf, 8192, 0)) > 0){
              main_buf[main_len] = '\0';
              // printf("Recive from server: %d\n", main_len);
              // printf("Recive from server: %s\n", main_buf);
              int len = send(client_sockfd, main_buf, main_len, 0);
              printf("send message to client end: %d\n", len);
            }

            buf_pos = 0;
          }
        }
      }
    }
  }
  return 0;
}

