
#include "Utility.h"
#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#endif // defined(_WIN32) || defined(_WIN64)

#include "TinySSH.h"

int main(int argc, char* argv[])
{
	return TinySSHAppClient(argc, argv);
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
		return 0;
	}
	char buf[256] = { 0 };
	//send message to server
	while (true){
		memset(buf,0,256);
		printf("client>");
		fgets(buf,256,stdin);
		std::string input(buf);
		if (input.back() == '\n')
		{
			input.pop_back();
		}
		int ret = client.SendParams(input);
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
	return 0;
}
