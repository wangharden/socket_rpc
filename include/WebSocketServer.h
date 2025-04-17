#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <functional>
#include <queue>
#include <vector>


class WebSocketServer {
public:
    WebSocketServer(const std::string& ip = "0.0.0.0", unsigned short port = 8080);
    ~WebSocketServer();

    bool Start();
    void Stop();
    bool SendToClient(int clientId, const std::string& message);
    void Broadcast(const std::string& message);

    typedef std::function<void(int clientId, const std::string&)> MessageHandler;
    void SetMessageHandler(MessageHandler handler) { messageHandler = handler; }

private:
    void HandleConnections();
    void HandleClient(int clientId);
    
    bool PerformHandshake(int clientSocket);
    
    std::string DecodeWebSocketFrame(const std::vector<unsigned char>& buffer);
    std::vector<unsigned char> EncodeWebSocketFrame(const std::string& message);

private:
    std::string ip;
    unsigned short port;
    int serverSocket;
    std::thread acceptThread;
    std::map<int, std::thread> clientThreads;
    std::mutex clientsMutex;
    std::map<int, bool> activeClients;
    bool running;
    MessageHandler messageHandler;
};

#endif // WEBSOCKET_SERVER_H