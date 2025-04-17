#include "Utility.h"
#include "RPCHandler.h"
#include <thread>
#include <sstream>
#include <array>
#include <iostream>

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

Reflect_Class(SSHApp);
Reflect_Class_Method(SSHApp, listall);
Reflect_Class_Method(SSHApp, login);
Reflect_Class_Method(SSHApp, connect);
Reflect_Class_Method(SSHApp, ssh);
Reflect_Class_Method(SSHApp, ssh_result);

int TinySSHAppServer(int argc, char* argv[])
{
    std::string ipstr = "0.0.0.0";
	USHORT port = 1998;
	if (argc >= 2){
		ipstr = argv[1];
	}
	if (argc >= 3){
		port = atoi(argv[2]);
	}
	BlockedServer server;
	bool res = server.Bind(ipstr, port);
	if (!res)
	{
		printf("Server bind ip %s:%d failed. Error code: %d\n",ipstr.c_str(),port, WGetLastError());
		return -1;
	}
	printf("Server bind ip %s:%d success\n",ipstr.c_str(),port);
	server.Listen(5);
	
	RPCHandler rpcHandler;
	SSHApp app;
    app.pHandle = &rpcHandler;
    app.pServer = &server;
	rpcHandler.Subscribe(&app);
	MsgCallback f = std::bind(&SSHApp::HandleMsg, &app, std::placeholders::_1, std::placeholders::_2);
	server.RegisterFunction(f);
    UCallback g = std::bind(&SSHApp::HanldeDisconnect, &app, std::placeholders::_1, std::placeholders::_2);
	server.RegisterDisconnectFunction(g);
	while (true)
	{
		SOCKET s = server.Accept();
		server.Tick();
	}
	return 0;
}

#if defined(ENV_WIN)
#define popen _popen
#define pclose _pclose
#endif

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

#define LOCAL_MODE_TIP "local client>"
#define REMOTE_CMD_TIP "remote client>"
class SSHAppClient
{
public:
    RPCHandler* pHandle = nullptr;
    BlockedClient* pClient = nullptr;
    bool bLogin = false;
    bool bConnected = false;
    bool bCmdMode = false;
    std::function<void(std::string)> fOnGetSSHResult;
public:
    void HandleMsg(SOCKET s, const std::string& data)
    {
        if (pHandle)
        {
            pHandle->Dispatch(s, data);
        }
        if (!bCmdMode || !bConnected)
        {
            fputs(LOCAL_MODE_TIP, stdout);
        }
    }

    void login_result(bool _bLogin)
    {
        bLogin = _bLogin;
        if (bLogin)
            printf("login success\n");
        else
            printf("login failed\n");
    }

    void ssh(std::string cmd)
    {
        printf("%s\n", cmd.c_str());
        if (pClient)
        {
            std::string result = exec(cmd.c_str());
            printf("%s\n", result.c_str());
            pClient->SendParams("ssh_result", result);
        }
    }

    void ssh_result(std::string res)
    {
        printf("%s\n", res.c_str());
        if (fOnGetSSHResult)
        {
            fOnGetSSHResult(res);
        }
    }

    void connect_result(bool bConn)
    {
        bConnected = bConn;
        if (!bConnected)
        {
            bCmdMode = false;
        }
        if (bConnected)
            printf("connect remote client success\n");
        else
            printf("disconnect from remote client\n");
    }

    void disconnect_result(bool bDisconn)
    {
        if (bDisconn)
            printf("disconnect from remote client\n");
    }

    void listall_result(std::string res)
    {
        printf("%s\n",res.c_str());
    }
};

Reflect_Class(SSHAppClient);
Reflect_Class_Method(SSHAppClient, ssh);
Reflect_Class_Method(SSHAppClient, ssh_result);
Reflect_Class_Method(SSHAppClient, connect_result);
Reflect_Class_Method(SSHAppClient, disconnect_result);
Reflect_Class_Method(SSHAppClient, login_result);
Reflect_Class_Method(SSHAppClient, listall_result);

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

#define ENTER_CMD_KEY ":cmd"
#define QUIT_CMD_KEY ":quit"


void ShowClientUsage()
{
    std::string tipRows[] = {
        "Instructions for using the remote console client:",
        "1. Login command:                  login xxxxxx        The key xxxxxxx following login is the one you specified",
        "2. Connection command:             connect xxxxxx      xxxxxx is the key of the client you want to connect to. If the match is successful, you can remotely control the client. Enter cmd mode",
        "3. Exit cmd mode:                  :quit               Quit cmd command mode",
        "4. Enter cmd mode:                 :cmd                Enters cmd command mode",
        "5. Cancel connection:              disconnect          Cancel the connection to the currently connected client"
    };
    int rows = sizeof(tipRows) / sizeof(std::string);
    for (int i = 0;i < rows; ++i)
    {
        std::cout<<tipRows[i]<<std::endl;
    }
}

int TinySSHAppClient(int argc, char* argv[])
{
    std::string ipstr = "127.0.0.1";
	USHORT port = 1998;
	if (argc >= 2){
		ipstr = argv[1];
	}
	if (argc >= 3){
		port = atoi(argv[2]);
	}
	printf("client IP address: %s, port %d\n",ipstr.c_str(), port);
	BlockedClient client;
	bool b0 = client.Connect(ipstr, port);
	//connect to the server
	if (!b0){
		int err = WGetLastError();
		printf("connect failed. error code %d\n",err);
		return -1;
	}
    
    RPCHandler rpcHandler;
    SSHAppClient app;
    app.pHandle = &rpcHandler;
    app.pClient = &client;
    rpcHandler.Subscribe(&app);
    MsgCallback f = std::bind(&SSHAppClient::HandleMsg, &app, std::placeholders::_1, std::placeholders::_2);
    client.RegisterFunction(f);
    app.fOnGetSSHResult = [](std::string res){
        fputs(REMOTE_CMD_TIP, stdout);
    };
    std::thread tickThread([&](){
        while (true)
        {
            client.Tick();
        }
    });

    ShowClientUsage();

	char buf[256] = { 0 };
    fputs(LOCAL_MODE_TIP, stdout);
	//send message to server
	while (true){
		memset(buf,0,256);
		fgets(buf,256,stdin);
		std::string input(buf);
		if (input.back() == '\n')
		{
			input.pop_back();
		}
        
        int ret = 0;
        auto sendToServer = [&](){
            std::vector<std::string> args = split(input, ' ');
            std::string msg;
            for (auto& item: args)
            {
                msg += SockProto::Encode(item);
            }
            ret = client.Send(msg.data(), msg.size(), 0);
        };
        if (app.bConnected && app.bCmdMode)
        {
            if (input == QUIT_CMD_KEY)
            {
                app.bCmdMode = false;
                fputs(LOCAL_MODE_TIP, stdout);
            }
            else
            {
                ret = client.SendParams("ssh", input);
            }
        }
        else if (app.bConnected && !app.bCmdMode)
        {
            if (input == ENTER_CMD_KEY)
            {
                app.bCmdMode = true;
                fputs(REMOTE_CMD_TIP, stdout);
            }
            else
            {
                sendToServer();
            }
        }
        else
        {
            sendToServer();
        }
		if (ret < 0){
			printf("send message failed\n");
			bool b = client.ReConnect();
			if (!b) {
				printf("reconnect failed\n");
				break;
			}
			else {
				printf("reconnect success\n");
			}
		}
	}
    tickThread.join();
	return 0;
}