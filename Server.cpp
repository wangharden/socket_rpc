#include "Utility.h"
#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#endif // defined(_WIN32) || defined(_WIN64)
#include "GobangGame.h"
#include "Landlord.h"
#include "RPCHandler.h"
#include "TinySSH.h"
class Person
{
	int age = 0;
public:
	Person() {};
	Person(int _age) : age(_age) {}
	void hello()
	{
		printf("hello my socket\n");
	}

	void laugh()
	{
		printf("hhhhhhh, my age %d\n",age);
	}
};

Reflect_Class(Person);
Reflect_Class_Method(Person, hello);
Reflect_Class_Method(Person, laugh);

int main(int argc, char* argv[])
{
#if 1
	return TinySSHAppServer(argc, argv);
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
	Person p;
	Person p1(24);
	rpcHandler.Subscribe(&p);
	rpcHandler.Subscribe(&p1, "hello");
	auto f = std::bind(&RPCHandler::Dispatch, &rpcHandler, std::placeholders::_1, std::placeholders::_2);
	server.RegisterFunction(f);
	server.RegisterDisconnectFunction([](SOCKET s, sockaddr_in addr4) {
		std::string ip;
		int port;
		get_ip_port(addr4, ip, port);
		printf("socket %lld disconnect, ip: %s:%d\n", s, ip.c_str(),port);
		});
	while (true)
	{
		SOCKET s = server.Accept();
		server.Tick();
	}
	return 0;
#else
	// RoomServer roomServer;
	// std::vector<SOCKET> room;
	// std::string roomName;
	// std::string IP = "0.0.0.0";
	// int Port = 1009;
	// bool res = roomServer.Bind(IP, Port);
	// if (!res) {
	// 	int err = WGetLastError();
	// 	printf("bind failed. IP: %s, Port: %d, Error Code: %d\n", IP.c_str(), Port, err);
	// 	return -1;
	// }
	// roomServer.Listen(5);
	// while (true)
	// {
	// 	roomServer.Tick();
	// 	bool bRoomReady = roomServer.GetReadyRoom(room, roomName);
	// 	if (bRoomReady)
	// 	{
	// 		SOCKET c0 = room[0];
	// 		SOCKET c1 = room[1];
	// 		printf("get first client: %d\n", c0);
	// 		printf("get second client: %d\n", c1);
	// 		pid_t pid = fork();
	// 		if (pid == 0) {
	// 			//child process
	// 			printf("launch child process\n");
	// 			roomServer.CloseServer();
	// 			GobangGame game(c0, c1);
	// 			game.Start();
	// 			exit(EXIT_SUCCESS);
	// 		}
	// 		else if (pid > 0 )
	// 		{
	// 			//parent process
	// 			//roomServer.CloseConnection(c0);
	// 			//roomServer.CloseConnection(c1);
	// 		    roomServer.MarkRoomLaunch(roomName, true, pid);
	// 		}
	// 	}
	// }
	// return 0;
#endif
}
