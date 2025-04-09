#include "Utility.h"
#include "Game.h"
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <locale>
#include <codecvt>
#include <string>
class WSAObject
{
#if defined(_WIN32) || defined(_WIN64)
    WORD m_version;
    WSADATA m_wsaData;
public:
    WSAObject(BYTE majorVersion, BYTE minorVersion) : m_version(MAKEWORD(majorVersion, minorVersion)) {
        if (!this->Initialize())
        {
            assert(false);
        }
    }
    WSAObject() : WSAObject(2, 2) {}
    bool Initialize()
    {
        return WSAStartup(m_version, &m_wsaData) == 0;
    }
    ~WSAObject()
    {
        WSACleanup();
    }
#endif
};

static WSAObject wsaObj;

int WGetLastError()
{
#if defined(_WIN32) || defined(_WIN64)
    return WSAGetLastError();
#else
    return errno;
#endif
}

//return true if last error equals EAGAIN on linux, or WSAEWOULDBLOCK on windows
bool NET_EWOULDBLOCK()
{
    int err = WGetLastError();
#if defined(ENV_WIN)
    return err == WSAEWOULDBLOCK;
#else
    return err == EWOULDBLOCK;
#endif
}

bool BlockedServer::HasClientConnect() const
{
    for (auto& s : m_acceptsock) {
        if (s.state == CONNECTED)
            return true;
    }
    return false;
}

void BlockedServer::CloseClient(size_t sockID)
{
    ClientSock& s = m_acceptsock[sockID];
    s.state = CLOSED;
    Log("BlockedServer::CloseClient socket %lld is closed\n", s.socket);
    CloseSocket(s.socket);
    if (s.disconnect_callback)
    {
        s.disconnect_callback(s.socket);
    }
    else if (m_fDisconnectHandle)
    {
        m_fDisconnectHandle(s.socket);
    }
    s.Init();
}

int BlockedServer::FindSocketID(SOCKET s) const
{
    for (size_t i = 0; i < m_acceptsock.size(); ++i)
    {
        if (m_acceptsock[i].socket == s) {
            return (int)i;
        }
    }
    return -1;
}

bool BlockedServer::ReceiveData(size_t sockID, int len)
{
    int mLen = std::min<int>(m_MTU, len);
    bool finished = false;
    std::string buffstr;
    buffstr.resize(m_MTU);
    char* buffer = (char*)buffstr.data();
    int numGet = 0;
    int sumGet = 0;
    ClientSock& s = m_acceptsock[sockID];
    while (!finished)
    {
        try
        {
            numGet = this->Recieve(s.socket, buffer, mLen, 0);
            sumGet += numGet;
            if (numGet > 0)
            {
                for (int i = 0; i < numGet; ++i)
                    s.datacache.push_back(buffer[i]);
                mLen = len - sumGet;
                finished = mLen == 0;
                mLen = std::min<int>(m_MTU, len);
            }
            else if (numGet < 0)
            {
                int err = WGetLastError();
                if (NET_EWOULDBLOCK())
                {
                    break;
                }
                else
                {
                    Log("BlockedServer::ReceiveData socket %lld is disconnected, net error code: %d\n", s.socket, err);
                    s.state = CLOSED;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        catch (std::exception e)
        {
            s.state = CLOSED;
            printf("BlockedServer::ReceiveData error: %s", e.what());
            break;
        }
    }
    return finished;
}

void BlockedServer::PushClientSock(const ClientSock& csock)
{
    ClientSock* psock = nullptr;
    for (auto& item : m_acceptsock)
    {
        if (item.state == CLOSED)
        {
            item = csock;
            Log("BlockedServer::PushClientSock find an empty spot, push to it\n");
            psock = &item;
            break;
        }
    }
    if (!psock)
    {
        m_acceptsock.push_back(csock);
        psock = &m_acceptsock.back();
    }
    get_ip_port(psock->sockaddr, psock->ip, psock->port);
    Log("BlockedServer::PushClientSock client ip: %s, port: %d\n", psock->ip.c_str(), psock->port);
}

void BlockedServer::Tick()
{
    if (!HasClientConnect())
        return;
    for (size_t i = 0; i < m_acceptsock.size(); ++i)
    {
        ClientSock& s = m_acceptsock[i];
        if (s.state == CLOSED)
            continue;

	    if (!this->ShouldReceiveData(s.socket))
	        continue;
        if (s.explen == 0)
        {
            bool finished = ReceiveData(i, sizeof(int));
            if (finished)
            {
                s.explen = SockProto::ByteToInt32(s.datacache);
                s.datacache.clear();
                Log("BlockedServer Start receive new package, expected size: %d\n", s.explen);
            }
            else if (s.state == CLOSED)
            {
                CloseClient(i);
            }
        }
        else
        {
            bool finished = ReceiveData(i, s.explen - (int)s.datacache.size());
            if (finished)
            {
                Log("BlockedServer End receive new package, package size: %d\n", s.explen);
                if (s.callback)
                {
                    s.callback(s.socket, s.datacache);
                }
                else if (m_fMsgHandle)
                {
                    m_fMsgHandle(s.socket, s.datacache);
                }
                s.datacache.clear();
                s.explen = 0;
            }
            else if (s.state == CLOSED)
            {
                CloseClient(i);
            }
        }
    }
}

int BlockedServer::AliveClientCount() const
{
    int num = 0;
    for (auto& s : m_acceptsock)
    {
        if (s.state == CONNECTED) ++num;
    }
    return num;
}

bool BlockedClient::ReceiveData(int len)
{
    int mLen = std::min<int>(m_MTU, len);
    bool finished = false;
    std::string buffstr;
    buffstr.resize(m_MTU);
    char* buffer = (char*)buffstr.data();
    int numGet = 0;
    int sumGet = 0;
    ClientSock& s = m_client;
    while (!finished)
    {
        try
        {
            numGet = this->Recieve(buffer, mLen, 0);
            sumGet += numGet;
            if (numGet > 0)
            {
                for (int i = 0; i < numGet; ++i)
                    s.datacache.push_back(buffer[i]);
                mLen = len - sumGet;
                finished = mLen == 0;
                mLen = std::min<int>(m_MTU, len);
            }
            else if (numGet < 0)
            {
                int err = WGetLastError();
                if (NET_EWOULDBLOCK())
                {
                    break;
                }
                else
                {
                    Log("BlockedClient::ReceiveData socket %lld is disconnected, net error code: %d\n", s.socket, err);
                    s.state = CLOSED;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        catch (std::exception e)
        {
            s.state = CLOSED;
            printf("BlockedClient::ReceiveData error: %s", e.what());
            break;
        }
    }
    return finished;
}

void BlockedClient::Tick()
{
    if (!IsConnected())
        return;
    ClientSock& s = m_client;

    if (s.explen == 0)
    {
        bool finished = ReceiveData(sizeof(int));
        if (finished)
        {
            s.explen = SockProto::ByteToInt32(s.datacache);
            s.datacache.clear();
            Log("BlockedClient Start receive new package, expected size: %d\n", s.explen);
        }
        else if (s.state == CLOSED)
        {
            Close();
        }
    }
    else
    {
        bool finished = ReceiveData(s.explen - (int)s.datacache.size());
        if (finished)
        {
            Log("BlockedClient End receive new package, package size: %d\n", s.explen);
            if (s.callback)
            {
                s.callback(s.socket, s.datacache);
            }
            s.datacache.clear();
            s.explen = 0;
        }
        else if (s.state == CLOSED)
        {
            Close();
        }
    }
}
