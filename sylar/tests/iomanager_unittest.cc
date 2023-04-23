#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = LOG_ROOT();
int                sock     = 0;

void test_fiber() {
  LOG_INFO(g_logger) << "test_fiber sock=" << sock;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);
  sockaddr_in addr;
  memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(80);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

  if (!connect(sock, (const sockaddr *)&addr, sizeof(addr))) {
  } else if (errno == EINPROGRESS) {
    LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, []() {
      LOG_INFO(g_logger) << "read callback";
    });
    sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, []() {
      LOG_INFO(g_logger) << "write callback";
      sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
      close(sock);
    });
  } else {
    LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
  }
}

void test1() {
  std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;
  sylar::IOManager iom(2, false);
  iom.schedule(&test_fiber);
}

sylar::Timer::ptr s_timer;
void              test_timer() {
  sylar::IOManager iom(2);

  s_timer = iom.addTimer(
      1000,
      []() {
        static int i = 0;
        LOG_INFO(g_logger) << "hello timer i=" << i;
        if (++i == 3) {
          s_timer->reset(2000, true);
        }
      },
      true);
}

int main(int argc, char **argv) {
  // test_timer();
  test1();
  return 0;
}
