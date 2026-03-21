///@system  Dstar V10 api demo
///@file    UdpClient.cpp
///@author  Hao Lin 2021-01-20

#ifndef TUDPCLIENT_H
#define TUDPCLIENT_H

#if defined WIN32 || defined _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")

#else

#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

#endif


class TUdpClient
{
public:
    TUdpClient();
    ~TUdpClient();

    int  Init(const char *toip, const unsigned short toport);
    void Close();

    int Send(const char* buf, const int len);//返回0为成功

private:
#if defined WIN32 || defined _WIN32
    SOCKET m_Socket;
#else
    int         m_Socket;
#endif
    sockaddr_in m_ToAddr;
};

#endif // TUDPCLIENT_H
