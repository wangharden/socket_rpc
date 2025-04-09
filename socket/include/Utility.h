#pragma once
#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ENV_WIN
#include <ws2tcpip.h>
#include <WinSock2.h>
#else
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define ENV_UNIX
#define USHORT unsigned short
#define MAX_PATH 260
#define SOCKET long long int
#define SOCKET_ERROR -1
#define BYTE unsigned char
#endif
#include <iostream>
#include <string.h>
#include <vector>
#include <functional>
#include <map>
#include "LightProto.h"


inline sockaddr_in create_sockaddr(USHORT fam, std::string ipAddr, USHORT port)
{
    sockaddr_in addr;
    addr.sin_family = fam;
    addr.sin_addr.s_addr = inet_addr(ipAddr.c_str());
    addr.sin_port = htons(port);
    return addr;
}

inline void get_ip_port(sockaddr_in& addr, std::string& ip, int& port)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    ip = client_ip;
    port = ntohs(addr.sin_port);
}

inline std::string getIP()
{
/*
    char hostName[MAX_PATH];
    if (!gethostname(hostName, sizeof(hostName)))
    {
        hostent* host = gethostbyname(hostName);
        if (host != nullptr)
        {
            std::string ipstr = inet_ntoa(*(in_addr*)*host->h_addr_list);
            return ipstr;
        }
    }*/
    return "";
}
inline void CloseSocket(SOCKET s)
{
#if defined(ENV_WIN)
	closesocket(s);
#else
	close(s);
#endif
}
inline int SetSocketNonBlocking(SOCKET s)
{
#if defined(ENV_WIN)
    u_long mode = 1;        //1:nonblocking 0: blocking
    int result = ioctlsocket(s, FIONBIO, &mode);
    if (result == SOCKET_ERROR)
        return -1;
    return 0;
#else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(s, F_SETFL, flags) == -1) {
        return -1;
    }
    return 0;
#endif
}

int WGetLastError();

enum
{
    CLOSED,
    CONNECTED,
};
typedef std::function<void(SOCKET, const std::string&)> MsgCallback;
typedef std::function<void(SOCKET s)> UCallback;
struct ClientSock
{
    SOCKET socket;
    BYTE state = CLOSED;
    sockaddr_in sockaddr;
    std::string datacache = "";
    int explen = 0;                 //expected data length
    std::string ip = "";            //client ip address
    int port = -1;                  //client port
    MsgCallback callback;             //msg handler
    UCallback disconnect_callback;  //disconnection callback
    void Init() {
        state = CLOSED;
        datacache = "";
        explen = 0;
    }
};

class BlockedServer
{
    
protected:
    SOCKET m_sock = SOCKET_ERROR;
    sockaddr_in m_sockaddr;
    int m_MTU = 1500;
    std::vector<ClientSock> m_acceptsock;
    MsgCallback m_fMsgHandle;
    UCallback m_fDisconnectHandle;
protected:
    bool HasClientConnect()const;
    void CloseClient(size_t sockID);
    void CloseServer() {CloseSocket(m_sock);}
    int FindSocketID(SOCKET s)const;
    virtual bool ShouldReceiveData(SOCKET s)const { return true; }
    bool ReceiveData(size_t sockID, int len);
    void PushClientSock(const ClientSock& csock);
public:
    BlockedServer()
    {
        m_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sock != SOCKET_ERROR)
        {
            SetSocketNonBlocking(m_sock);
        }
    }
    bool Bind(const std::string &ipAddr, USHORT port)
    {
        m_sockaddr = create_sockaddr(PF_INET, ipAddr.c_str(), port);
        int ret = ::bind(m_sock, (sockaddr *)&m_sockaddr, sizeof(sockaddr_in));
        return ret != SOCKET_ERROR;
    }
    bool Listen(int backlog = 1)
    {
        int ret = listen(m_sock, backlog);
        return ret != SOCKET_ERROR;
    }
    SOCKET Accept(sockaddr *addr = nullptr)
    {
        ClientSock c;
        socklen_t addr_len = sizeof(c.sockaddr);
        SOCKET csock = accept(m_sock,(sockaddr*)&c.sockaddr, &addr_len);
        if (csock != SOCKET_ERROR)
        {
            c.state = CONNECTED;
            c.socket = csock;
            PushClientSock(c);
            SetSocketNonBlocking(csock);
            if (addr)
            {
                *addr = *((sockaddr*)&c.sockaddr);
            }
        }
        return csock;
    }
    /*
    * Register callback for specific client socket.
    * When client socket get a message, it will invoke callback function.
    */
    bool RegisterFunction(SOCKET target, MsgCallback fun)
    {
        for (auto& s : m_acceptsock)
        {
            if (s.socket == target)
            {
                s.callback = fun;
                return true;
            }
        }
        return false;
    }
    /*
    * Register default callback function for all client.
    * if client socket not registered a callback, when it get messages, will invoke default callback function.
    */
    bool RegisterFunction(MsgCallback defaultCallback)
    {
        m_fMsgHandle = defaultCallback;
        return true;
    }
    /*
    * Register Disconnect callback for specific socket.
    */
    bool RegisterDisconnectFunction(SOCKET target, UCallback fun)
    {
        for (auto& s : m_acceptsock)
        {
            if (s.socket == target)
            {
                s.disconnect_callback = fun;
                return true;
            }
        }
        return false;
    }
    /*
    * Register Default Disconnect callback for all socket.
    */
    bool RegisterDisconnectFunction(UCallback fun)
    {
        m_fDisconnectHandle = fun;
        return true;
    }
    void TakeOver(SOCKET s, BYTE state = CONNECTED) {
        if (s != SOCKET_ERROR)
        {
            PushClientSock({ s, state });
            SetSocketNonBlocking(s);
        }
    }
    int Send(SOCKET s, const char *buf, int len, int flags)
    {
	    int i = FindSocketID(s);
	    if (m_acceptsock[i].state == CLOSED) return -1;
#if defined(ENV_WIN)
        int num = send(s, buf, len, flags);
#else
        int num = send(s, buf, len, MSG_NOSIGNAL);
#endif
        if (num < 0)
        {
            m_acceptsock[i].state = CLOSED;
        }
        return num;
    }
    template<class ...Args>
    int SendParams(SOCKET s, Args&& ...args)
    {
        std::string data = SockProto::Encode(args...);
        int size = (int)data.size();
        std::string dataSize = SockProto::Int32ToByte(size);
        if (this->Send(s, dataSize.data(), sizeof(int), 0) != sizeof(int) )
        {
	        LogError("BlockServer::SendParams Send head failed\n");
            return SOCKET_ERROR;
        }
	    int num = 0;
	    int sum = 0;
	    while (sum < size) 
	    {
	        num = this->Send(s, data.data() + sum, size - sum, 0);
	        if (num < 0){
                LogError("BlockServer::SendParams Send body failed\n");
		        return SOCKET_ERROR;
	        }
	        sum += num;
	    }
	    return 0;
    }
    int Recieve(SOCKET s, char *buf, int len, int flags)
    {
        return recv(s, buf, len, flags);
    }
    void CloseConnection(SOCKET s)
    {
	    int i = FindSocketID(s);
	    if (i!=-1)
	    {
		    CloseClient(i);
	    }
    }
    virtual void Tick();
    int AliveClientCount()const;
    virtual ~BlockedServer()
    {
        for (size_t i = 0; i < m_acceptsock.size(); ++i)
        {
            CloseClient(i);
        }
        CloseServer();
    }
};




class BlockedClient
{
    ClientSock m_client;
    int m_MTU = 1500;
public:
    BlockedClient()
    {
        m_client.socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    bool Connect(const std::string &ipStr, USHORT port)
    {
        m_client.sockaddr = create_sockaddr(PF_INET, ipStr.c_str(), port);
        int ret = connect(m_client.socket, (sockaddr *)&m_client.sockaddr, sizeof(sockaddr_in));
        if (ret != SOCKET_ERROR)
        {
            m_client.state = CONNECTED;
        }
        return ret != SOCKET_ERROR;
    }
    int Send(const char *buf, int len, int flags)
    {
        return send(m_client.socket, buf, len, flags);
    }
    int Recieve(char *buf, int len, int flags)
    {
        return recv(m_client.socket, buf, len, flags);
    }
    bool ReConnect()
    {
        m_client.socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        int ret = connect(m_client.socket, (sockaddr *)&m_client.sockaddr, sizeof(sockaddr_in));
        return ret != SOCKET_ERROR;
    }
    void Close() {
        CloseSocket(m_client.socket);
        m_client.state = CLOSED;
    }
    ~BlockedClient()
    {
        Close();
    }
    template<class ...Args>
    int SendParams(Args&& ...args)
    {
        std::string data = SockProto::Encode(args...);
        int size = (int)data.size();
        std::string dataSize = SockProto::Int32ToByte(size);
        if (this->Send(dataSize.data(), sizeof(int), 0) != sizeof(int))
        {
            Log("BlockedClient::SendParams Send head failed\n");
            return SOCKET_ERROR;
        }
        int num = 0;
        int sum = 0;
        while (sum < size)
        {
            num = this->Send(data.data() + sum, size - sum, 0);
            if (num < 0) {
                Log("BlockedClient::SendParams Send body failed\n");
                return SOCKET_ERROR;
            }
            sum += num;
        }
        return 0;
    }
    bool IsConnected()const { return m_client.state == CONNECTED; }
    bool ReceiveData(int len);
    void Tick();
};
