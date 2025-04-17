// server.cpp (整合版 - Removed SSHApp Reflect Macros)

#include "Utility.h"
#if defined(_WIN32) || defined(_WIN64)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#endif // defined(_WIN32) || defined(_WIN64)

// 可能需要包含的头文件，取决于你的项目结构和 RPC 目标
// #include "GobangGame.h"
// #include "Landlord.h"
#include "RPCHandler.h"
#include "TinySSH.h"         // 需要包含它来获取 SSHApp 类的定义

#include "WebSocketServer.h" // 包含新的 WebSocket 服务器头文件

#include <vector>
#include <thread>         // For std::thread, std::jthread
#include <mutex>          // For std::mutex
#include <condition_variable> // For std::condition_variable
#include <queue>          // For std::queue
#include <iostream>       // For std::cout, std::cerr
#include <string>         // For std::string
#include <atomic>         // For std::atomic_bool
#include <csignal>        // For signal handling (Ctrl+C)
#include <chrono>         // For sleep
#include <memory>         // For smart pointers if needed

// --- Globals ---
// 使用智能指针管理 WebSocketServer，避免全局裸指针问题
std::unique_ptr<WebSocketServer> g_wsServerInstance;
std::mutex g_wsServerMutex; // Mutex to protect access to g_wsServerInstance if needed outside its thread
// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested = false;

// --- Original RPC/Person Class (保持不变) ---
class Person
{
	int age = 0;
public:
	Person() {};
	Person(int _age) : age(_age) {}
	void hello()
	{
		printf("Person::hello() called\n");
	}

	void laugh()
	{
		printf("Person::laugh() called, my age %d\n", age);
	}
};

// Person 类的反射宏应该放在这里，或者一个单独的注册文件/Person.cpp 中
Reflect_Class(Person);
Reflect_Class_Method(Person, hello);
Reflect_Class_Method(Person, laugh);




// 处理通过 WebSocket 接收到的消息
void HandleWebSocketMessage(int clientId, const std::string& message) {
    std::cout << "[WebSocket] Received from client " << clientId << ": " << message << std::endl;

    // 使用锁保护对全局实例的访问（尽管在此回调中可能不是严格必需的，但更安全）
    std::lock_guard<std::mutex> lock(g_wsServerMutex);
    if (g_wsServerInstance) { // 检查实例是否存在
        // 示例：回显消息给发送者
        std::string response = "Server acknowledges: " + message;
        if (!g_wsServerInstance->SendToClient(clientId, response)) {
            std::cerr << "[WebSocket] Failed to send echo to client " << clientId << std::endl;
        }

        // 示例：广播给所有 WebSocket 客户端（可选）
        // std::string broadcastMsg = "Client " + std::to_string(clientId) + " sent: " + message;
        // g_wsServerInstance->Broadcast(broadcastMsg);
    } else {
        std::cerr << "[WebSocket] Error: WebSocketServer instance is null in message handler!" << std::endl;
    }
}

// 在其自己的线程中运行 WebSocket 服务器的函数
void RunWebSocketServer(const std::string& ip, unsigned short port) {
    // 在此线程的栈上创建服务器实例
    WebSocketServer wsServer(ip, port);

    // 设置全局指针（在服务器启动前）
    {
        std::lock_guard<std::mutex> lock(g_wsServerMutex);
        // 直接重置 unique_ptr 是不行的，因为实例在栈上。
        // 这里使用裸指针可能更简单，但要注意生命周期。
        // 或者将 wsServer 也设为全局 unique_ptr 并在 main 中创建。
        // 为了简单起见，我们暂时恢复使用全局裸指针，并在 Stop 时置空。
        // g_wsServer = &wsServer; // (恢复使用全局裸指针 - 需谨慎)
        // **更好的方法:** 将 WebSocketServer 的创建移到 main 中，并传递引用或指针给线程。
        // **折中方案:** 仍然使用全局 unique_ptr，但在 main 中创建它。

        // 此处保持回调设置，假设 g_wsServerInstance 在 main 中被正确设置
    }

    // 获取全局实例的引用来设置处理器（假设它在 main 中被设置）
    WebSocketServer* currentServer = nullptr;
    {
         std::lock_guard<std::mutex> lock(g_wsServerMutex);
         currentServer = g_wsServerInstance.get();
    }

    if (!currentServer) {
         std::cerr << "[WebSocket] Error: Server instance not set before thread start!" << std::endl;
         return;
    }

    currentServer->SetMessageHandler(HandleWebSocketMessage);

    if (!currentServer->Start()) {
        std::cerr << "[WebSocket] Server failed to start on " << ip << ":" << port << std::endl;
        // 不需要手动置空 unique_ptr，它在 main 中管理
        return;
    }
    std::cout << "[WebSocket] Server listening on " << ip << ":" << port << std::endl;

    // 保持线程活动，直到请求关闭
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 请求关闭
    std::cout << "[WebSocket] Shutdown requested. Stopping server..." << std::endl;
    currentServer->Stop(); // 停止 WebSocket 服务器
    std::cout << "[WebSocket] Server stopped." << std::endl;
}

// --- 信号处理，用于优雅关停 ---
void handle_signal(int signum) {
    if (signum == SIGINT) { // 处理 Ctrl+C
        std::cout << "\nCtrl+C detected. Requesting shutdown..." << std::endl;
        g_shutdown_requested.store(true);
    }
}

// --- 主函数 ---
int main(int argc, char* argv[])
{
    // 默认端口
	std::string ipstr = "0.0.0.0";
	USHORT rpc_port = 1998; // RPC 服务器端口
	USHORT ws_port = 8080;  // WebSocket 服务器端口

    // 简单的命令行参数解析
	if (argc >= 2){
		ipstr = argv[1];
	}
	if (argc >= 3){
		try { rpc_port = static_cast<USHORT>(std::stoi(argv[2])); }
        catch(...) { std::cerr << "invalid RPC : " << argv[2] << std::endl; return 1; }
	}
	if (argc >= 4){
		try { ws_port = static_cast<USHORT>(std::stoi(argv[3])); }
        catch(...) { std::cerr << "invalid WebSocket : " << argv[3] << std::endl; return 1; }
	}

    // 注册 Ctrl+C 信号处理器
    signal(SIGINT, handle_signal);

	// --- 创建和启动 WebSocket 服务器 ---
    {
        std::lock_guard<std::mutex> lock(g_wsServerMutex);
        g_wsServerInstance = std::make_unique<WebSocketServer>(ipstr, ws_port);
    }
	std::jthread wsThread(RunWebSocketServer, ipstr, ws_port); // 使用 jthread 自动管理
	std::cout << "[Main] WebSocket server thread starting..." << std::endl;


	// --- 创建和启动原始 RPC (BlockedServer) ---
	BlockedServer rpc_server;
	bool res = rpc_server.Bind(ipstr, rpc_port);
	if (!res)
	{
		std::cerr << "[RPC Server] Bind failed on " << ipstr << ":" << rpc_port << ". Error code: " << WGetLastError() << std::endl;
		g_shutdown_requested.store(true); // 通知 WebSocket 线程停止
		// wsThread 会在 main 结束时自动 join
		return -1;
	}
	std::cout << "[RPC Server] Bound successfully to " << ipstr << ":" << rpc_port << std::endl;

	if (!rpc_server.Listen(5)) {
        std::cerr << "[RPC Server] Listen failed. Error code: " << WGetLastError() << std::endl;
        g_shutdown_requested.store(true); // 通知 WebSocket 线程停止
		return -1;
    }
    std::cout << "[RPC Server] Listening for RPC connections..." << std::endl;

	// 设置 RPCHandler 并订阅对象
	RPCHandler rpcHandler;
	Person p;
	Person p1(24);
	rpcHandler.Subscribe(&p);       // 订阅默认的 Person 实例的所有反射方法
	rpcHandler.Subscribe(&p1, "hello"); // 订阅 p1 实例，但只处理 "hello" 方法调用

    // **重要:** 创建并订阅 SSHApp 实例，以便 RPC 系统可以调用它的方法
    // 即使 TinySSHAppServer 不再是主入口点
    SSHApp ssh_app_instance;
    ssh_app_instance.pServer = &rpc_server; // SSHApp 需要指向服务器的指针
    // ssh_app_instance.pHandle = &rpcHandler; // 如果 SSHApp 内部需要 RPCHandler
    rpcHandler.Subscribe(&ssh_app_instance); // 订阅 ssh_app_instance 的所有反射方法
                                             // (假设 SSHApp 的反射宏在 TinySSH.cpp 中)

	// 为 RPC 服务器注册消息分发函数
	auto f = std::bind(&RPCHandler::Dispatch, &rpcHandler, std::placeholders::_1, std::placeholders::_2);
	rpc_server.RegisterFunction(f);

    // 为 RPC 服务器注册断开连接处理器
	rpc_server.RegisterDisconnectFunction([](SOCKET s, sockaddr_in addr4) {
		std::string ip;
		int port;
		get_ip_port(addr4, ip, port); // 假设 get_ip_port 在 Utility.h 中可用
		printf("[RPC Server] Client socket %lld disconnected (ip: %s:%d)\n", s, ip.c_str(), port);
		});

	// --- RPC 服务器主循环 ---
	std::cout << "[Main] Entering main loop (RPC Accept & Tick). Press Ctrl+C to exit." << std::endl;
	while (!g_shutdown_requested.load())
	{
		// 接受新的 RPC 连接 (非阻塞)
		SOCKET new_rpc_socket = rpc_server.Accept();
        if (new_rpc_socket != SOCKET_ERROR) {
             printf("[RPC Server] Accepted new connection: socket %lld\n", new_rpc_socket);
        }

		// 处理现有的 RPC 连接 (读取数据, 处理消息/断开)
		rpc_server.Tick();

        // 短暂休眠，避免主循环空转消耗过多 CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// --- 程序关停 ---
	std::cout << "[Main] Shutdown requested. Stopping servers..." << std::endl;

    // RPC 服务器的关闭逻辑 (理想情况下 BlockedServer 应有 Stop 方法)
    // rpc_server.Stop(); // 调用 BlockedServer 的停止方法（如果实现）
    std::cout << "[Main] RPC Server processing stopped." << std::endl; // 假设 Tick 循环结束即停止处理

    // 等待 WebSocket 服务器线程结束 (jthread 会自动 join)
	std::cout << "[Main] Waiting for WebSocket server thread to finish..." << std::endl;
    // 如果使用 std::thread, 在这里调用 wsThread.join();

	std::cout << "[Main] Exiting." << std::endl;
	return 0;
}