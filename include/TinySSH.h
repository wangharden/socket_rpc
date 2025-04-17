// TinySSH.h (修改后)
#pragma once // 添加 include guard
#include "RPCHandler.h"
#include <thread>
#include <sstream>
#include <array>
#include <iostream>
#include <string>
#include <map>
#include "Utility.h"    // 需要 SOCKET, sockaddr_in 等
// 可能需要前向声明 BlockedServer 和 RPCHandler，如果它们定义比较复杂
// 且 SSHApp 定义中只需要指针。
class BlockedServer;
class RPCHandler;
class SSHApp
{
public:
    RPCHandler* pHandle = nullptr;
    BlockedServer* pServer = nullptr;
    SOCKET curMsgFrom = SOCKET_ERROR;
    std::map<SOCKET, std::string> pwdMap;           //socket -> password
    std::map<std::string, SOCKET> pwdMapInv;        //password -> socket
    std::map<SOCKET, SOCKET> conns;                 // controller -> controlled
    std::map<SOCKET, SOCKET> reverse_conns;         // controlled -> controller
public:
    void HandleMsg(SOCKET s, const std::string& msg)
    {
        curMsgFrom = s;
        if (pHandle)
        {
            pHandle->Dispatch(s, msg);
        }
    }

    void HanldeDisconnect(SOCKET s, sockaddr_in addr4)
    {
        if (pwdMap.find(s) != pwdMap.end())
        {
            pwdMapInv.erase(pwdMap[s]);
            pwdMap.erase(pwdMap.find(s));
        }
        if (conns.find(s) != conns.end())
        {
            conns.erase(conns.find(s));
        }
        if (reverse_conns.find(s) != reverse_conns.end())
        {
            reverse_conns.erase(reverse_conns.find(s));
        }
    }

    void listall()
    {
        char buf[256];
        SPRINTF_S(buf, "Password Map:\n");
        std::string res(buf);
        for (auto& item: pwdMap)
        {
            SPRINTF_S(buf, "%lld: %s\n", item.first,item.second.c_str());
            res += std::string(buf);
        }
        pServer->SendParams(curMsgFrom, "listall_result", res);
    }

    void login(std::string pwd)
    {
        if (curMsgFrom != SOCKET_ERROR)
        {
            if (pwdMapInv.find(pwd) == pwdMapInv.end())
            {
                pwdMap[curMsgFrom] = pwd;
                pwdMapInv[pwd] = curMsgFrom;
                Log("socket %lld login in with pwd: %s\n", curMsgFrom, pwd.c_str());
                pServer->SendParams(curMsgFrom, "login_result", true);
            }
            else
            {
                Log("socket %lld login failed, repeated pwd: %s\n", curMsgFrom, pwd.c_str());
                pServer->SendParams(curMsgFrom, "login_result", true);
            }
        }
    }

    void connect(std::string pwd)
    {
        bool bSuccess = false;
        if (pwdMapInv.find(pwd)!= pwdMapInv.end())
        {
            SOCKET target = pwdMapInv[pwd];
            if (curMsgFrom != target)
            {
                conns[curMsgFrom] = target;
                reverse_conns[target] = curMsgFrom;
                Log("socket %lld connect socket %lld success\n", curMsgFrom, target);
                bSuccess = true;
            }
        }
        pServer->SendParams(curMsgFrom, "connect_result", bSuccess);
    }

    void disconnect()
    {
        SOCKET source = curMsgFrom;
        if (conns.find(source) != conns.end())
        {
            SOCKET target = conns[source];
            conns.erase(source);
            reverse_conns.erase(target);
        }
        Log("socket %lld disconnect\n", source);
        pServer->SendParams(source, "disconnect_result", true);
    }

    void ssh(std::string cmd)
    {
        if (conns.find(curMsgFrom) != conns.end())
        {
            SOCKET targetClient = conns[curMsgFrom];
            if (pServer)
            {
                pServer->SendParams(targetClient, "ssh", cmd);
            }
        }
    }
    
    void ssh_result(std::string result)
    {
        if (reverse_conns.find(curMsgFrom) != reverse_conns.end())
        {
            SOCKET s = reverse_conns[curMsgFrom];
            if (pServer)
            {
                pServer->SendParams(s, "ssh_result", result);
            }
        }
    }
};


int TinySSHAppServer(int argc, char* argv[]);
/*
    远程控制台 客户端使用说明：
    1、登录指令：           login xxxxxxx          跟在login后面的xxxxxxx的是你指定的密钥
    2、连接指令：           connect xxxxxx         跟在connect后面的xxxxxx是你想连接的客户端的密钥，如果匹配成功，则能远程控制该客户端。进入cmd模式
    3、退出cmd模式指令：    :quit                  退出cmd指令模式
    4、进入cmd模式指令：    :cmd                   进入cmd指令模式
    5、取消连接指令：       disconnect             取消连接当前连接的客户端
*/
int TinySSHAppClient(int argc, char* argv[]);