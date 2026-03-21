///@system  Dstar V10 api demo
///@file    UdpClient.h
///@author  Hao Lin 2021-01-20

#include "UdpClient.h"

TUdpClient::TUdpClient()
{

#if defined WIN32 || defined _WIN32
    m_Socket = INVALID_SOCKET;
#else
    m_Socket = -1;
#endif
    
}

TUdpClient::~TUdpClient()
{
    Close();
}


int TUdpClient::Init(const char *toip, const unsigned short toport)
{
#if defined WIN32 || defined _WIN32
    //检查当前socket
    if (INVALID_SOCKET != m_Socket)
        return 1;
    //初始化Winsock
    WSADATA m_WsaData = { 0 };
    if (0 != ::WSAStartup(MAKEWORD(2, 2), &m_WsaData))
    {
        return 3;
    }
    //创建socket, 无效则返回
    SOCKET s = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == s)
        return 2;

    int val = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val));

    //设置阻塞
    unsigned long ul(0);
    ::ioctlsocket(s, FIONBIO, &ul);

    m_Socket = s;
       
#else
    //检查当前socket
    if (-1 != m_Socket)
        return 1;

    //创建socket, 无效则返回
    int s = ::socket(AF_INET,SOCK_DGRAM,0);
    if (-1 == s)
        return 2;
    
    unsigned value = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    m_Socket = s;

#endif
    memset(&m_ToAddr, 0, sizeof(sockaddr_in));
    m_ToAddr.sin_family = AF_INET;
    m_ToAddr.sin_addr.s_addr = inet_addr(toip);
    m_ToAddr.sin_port = htons(toport);
    return 0;
}

void TUdpClient::Close()
{
#if defined WIN32 || defined _WIN32
    if(INVALID_SOCKET != m_Socket)
    {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
    WSACleanup();
#else
    if (-1 == m_Socket)
        return;

    m_Socket = -1;
#endif
}

int TUdpClient::Send(const char *buf, const int len)
{
    int ret = ::sendto(m_Socket, buf, len, 0, (struct sockaddr*)&m_ToAddr, sizeof(m_ToAddr));
    if (len != ret)
    {
        return 1;
    }

    return 0;
}

