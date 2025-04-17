#include "WebSocketServer.h"
#include "Utility.h" // 假设需要
#include "sha1.hpp"  // <--- 在这里包含
#include "base64.h"  // <--- 在这里包含
#include <vector>
#include <string>
#include <iostream>
#include <sstream> // 用于 hex 转换
#include <iomanip> // 用于 hex 转换
#include <stdexcept> // 用于 hex 转换错误

#if defined(_WIN32) || defined(_WIN64)
// Windows Sockets includes (如果 Utility.h 没包含 recv/send/closesocket 等)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
// Linux/Unix Sockets includes
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


// --- 添加辅助函数：将十六进制字符串转换为字节 ---
std::vector<unsigned char> hex_string_to_bytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Hex string must have an even number of characters");
    }
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        try {
            // 使用 strtoul 将十六进制字符串转为无符号长整型，然后转为 char
            unsigned char byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
            bytes.push_back(byte);
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error("Invalid hex character in string: " + byteString);
        } catch (const std::out_of_range& e) {
             throw std::runtime_error("Hex value out of range: " + byteString);
        }
    }
    return bytes;
}
// 或者，如果你希望返回 std::string
std::string hex_string_to_binary_string(const std::string& hex) {
    std::string binary_string;
    if (hex.length() % 2 != 0) {
        throw std::runtime_error("Hex string must have an even number of characters");
    }
    binary_string.reserve(hex.length() / 2);
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
         try {
            unsigned char byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
            binary_string.push_back(static_cast<char>(byte));
        } catch (const std::exception& e) { // Catch broader exceptions
             throw std::runtime_error("Error converting hex byte '" + byteString + "': " + e.what());
        }
    }
    return binary_string;
}


// --- PerformHandshake 实现移到这里 ---
bool WebSocketServer::PerformHandshake(int clientSocket) {
    char buffer[2048] = {0};
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        std::cerr << "Handshake failed: Failed to read request or client disconnected." << std::endl;
        return false;
    }

    std::string request(buffer);
    std::string key;
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos != std::string::npos) {
        keyPos += 19;
        size_t keyEnd = request.find("\r\n", keyPos);
        if (keyEnd != std::string::npos) {
            key = request.substr(keyPos, keyEnd - keyPos);
        }
    }

    if (key.empty()) {
        std::cerr << "Handshake failed: Sec-WebSocket-Key not found or empty." << std::endl;
        return false;
    }

    std::string magicString = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    // 1. 计算 SHA-1 哈希 (返回十六进制字符串)
    SHA1 sha1;
    sha1.update(magicString);
    std::string sha1_hex_string = sha1.final(); // <--- 修改：调用无参数版本，接收返回的 hex string

    // 2. 将 Hex String 转换回二进制数据
    std::string sha1_binary_string;
    try {
        sha1_binary_string = hex_string_to_binary_string(sha1_hex_string);
         if (sha1_binary_string.length() != 20) {
             // 应该总是 20 字节 (40 hex chars)
             std::cerr << "Handshake error: SHA-1 result has unexpected length "
                       << sha1_binary_string.length() << " (hex: " << sha1_hex_string << ")" << std::endl;
             return false;
         }
    } catch (const std::runtime_error& e) {
        std::cerr << "Handshake error: Failed to convert SHA-1 hex to bytes: " << e.what() << std::endl;
        return false;
    }

    // 3. Base64 编码二进制数据
    std::string responseKey = base64_encode(sha1_binary_string);

    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + responseKey + "\r\n\r\n";

    int bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
    if (bytesSent < (int)response.length()) {
         std::cerr << "Handshake failed: Could not send complete response (" << bytesSent << "/" << response.length() << " bytes)." << std::endl;
        return false;
    }
    std::cout << "[WebSocket] Handshake successful for client " << clientSocket << std::endl;
    return true;
}

// --- 其他 WebSocketServer 的成员函数实现放在这里 ---
// 例如：构造函数、析构函数、Start、Stop、HandleConnections、HandleClient、
// DecodeWebSocketFrame、EncodeWebSocketFrame、SendToClient、Broadcast 等
// (确保 HandleClient 和 Stop 的线程安全修改也应用在此文件)
// ...
WebSocketServer::WebSocketServer(const std::string& ip, unsigned short port)
    : ip(ip), port(port), serverSocket(-1), running(false) {
}

WebSocketServer::~WebSocketServer() {
    // 确保 Stop 被调用
    if (running) {
        Stop();
    }
}

bool WebSocketServer::Start() {
    // ... Start 函数的实现 (与 WebSocketServer.cpp 中的保持一致) ...
    if (running) return true;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // ... (socket 创建、setsockopt、bind、listen 的错误检查) ...
     if (serverSocket < 0) {
        std::cerr << "创建WebSocket服务器套接字失败" << std::endl;
        return false;
    }
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str()); // Consider inet_pton for IPv6 compatibility later
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "WebSocket服务器绑定失败 (" << ip << ":" << port << ") Error: " << WSAGetLastError() << std::endl;
        #if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        #else
        close(serverSocket);
        #endif
        return false;
    }
    if (listen(serverSocket, SOMAXCONN) < 0) { // Use SOMAXCONN for backlog
        std::cerr << "WebSocket服务器监听失败 Error: " << WSAGetLastError() << std::endl;
        #if defined(_WIN32) || defined(_WIN64)
        closesocket(serverSocket);
        #else
        close(serverSocket);
        #endif
        return false;
    }


    running = true;
    std::cout << "WebSocket服务器启动中，准备监听 " << ip << ":" << port << std::endl;
    acceptThread = std::thread(&WebSocketServer::HandleConnections, this);

    return true;
}

void WebSocketServer::Stop() {
    // ... Stop 函数的实现 (应用上次的 detach 修改) ...
    if (!running) return;

    std::cout << "[WebSocket] Stop requested." << std::endl;
    running = false;

    if (serverSocket >= 0) {
        #if defined(_WIN32) || defined(_WIN64)
        shutdown(serverSocket, SD_BOTH);
        closesocket(serverSocket);
        #else
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        #endif
        serverSocket = -1;
         std::cout << "[WebSocket] Server socket closed." << std::endl;
    }

    if (acceptThread.joinable()) {
        // 如何唤醒 accept 线程？关闭 serverSocket 通常会让 accept 返回错误
        acceptThread.join();
         std::cout << "[WebSocket] Accept thread joined." << std::endl;
    }

    std::vector<int> clients_to_close;
    {
         std::lock_guard<std::mutex> lock(clientsMutex);
          std::cout << "[WebSocket] Closing active client sockets..." << std::endl;
         for (auto const& [clientId, isActive] : activeClients) {
             if (isActive) {
                 clients_to_close.push_back(clientId);
             }
         }
         activeClients.clear();
    }

     for(int sock : clients_to_close) {
         std::cout << "[WebSocket] Closing socket " << sock << std::endl;
         #if defined(_WIN32) || defined(_WIN64)
         shutdown(sock, SD_BOTH);
         closesocket(sock);
         #else
         shutdown(sock, SHUT_RDWR);
         close(sock);
         #endif
     }

    std::cout << "WebSocket服务器已停止" << std::endl;
}

void WebSocketServer::HandleConnections() {
     // ... HandleConnections 函数的实现 (应用上次的 detach 修改) ...
     while (running) { // Use the running flag
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        // accept 是阻塞的，当 serverSocket 关闭时，它会返回错误
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            if (running) { // 只有在服务器仍在运行时才报告错误
                std::cerr << "[WebSocket] Accept failed. Error: " << WSAGetLastError() << std::endl;
            } else {
                 std::cout << "[WebSocket] Accept loop exiting because server is stopping." << std::endl;
            }
            break; // 退出循环，线程结束
        }

        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        int clientPort = ntohs(clientAddr.sin_port);
        std::cout << "[WebSocket] New client connection: " << clientIP << ":" << clientPort << " (Socket: " << clientSocket << ")" << std::endl;


        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if (!running) {
                closesocket(clientSocket);
                continue;
            }
            activeClients[clientSocket] = true;
            try {
                std::thread clientThread(&WebSocketServer::HandleClient, this, clientSocket);
                clientThread.detach();
                std::cout << "[WebSocket] Detached thread for client " << clientSocket << std::endl;
            } catch (const std::system_error& e) {
                 std::cerr << "Failed to create thread for client " << clientSocket << ": " << e.what() << std::endl;
                 activeClients.erase(clientSocket);
                 closesocket(clientSocket);
            }
        }
    }
     std::cout << "[WebSocket] Accept thread finished." << std::endl;
}

void WebSocketServer::HandleClient(int clientSocket) {
     // ... HandleClient 函数的实现 (应用上次的 detach 修改) ...
    bool handshakeSuccess = false;
    // 在 PerformHandshake 前先检查服务器是否还在运行
    if (!running) {
         closesocket(clientSocket);
         return; // Server stopping, don't proceed
    }

    if (PerformHandshake(clientSocket)) {
        handshakeSuccess = true;
        std::vector<unsigned char> buffer(2048, 0); // Use a reasonable buffer size
        while (running) { // Check running flag in loop

            // Consider using select() or non-blocking sockets for better shutdown responsiveness
             int bytesRead = recv(clientSocket, (char*)buffer.data(), buffer.size(), 0);

            if (!running) break; // Check again after recv returns

            if (bytesRead <= 0) {
                 int error = WSAGetLastError();
                 bool shouldBreak = true;
                 if (bytesRead < 0 && error == WSAEWOULDBLOCK) {
                     #if defined(_WIN32) || defined(_WIN64)
                         // Non-blocking socket without data, continue or sleep
                         std::this_thread::sleep_for(std::chrono::milliseconds(10));
                         shouldBreak = false;
                     #else
                         // On Linux, non-blocking might return EAGAIN or EWOULDBLOCK
                         if (errno == EAGAIN || errno == EWOULDBLOCK) {
                             std::this_thread::sleep_for(std::chrono::milliseconds(10));
                             shouldBreak = false;
                         } else {
                             std::cerr << "[WebSocket] recv error for client " << clientSocket << ": " << strerror(errno) << std::endl;
                         }
                     #endif
                 } else {
                     // bytesRead == 0 (graceful close) or other error
                     std::cout << "[WebSocket] Client " << clientSocket << " disconnected (recv returned " << bytesRead << ", error: " << error << ")." << std::endl;
                 }
                 if(shouldBreak) break; // Exit loop on disconnect or error
                 else continue; // Continue if it was just EWOULDBLOCK
            }

            // Process received data
            std::string message = DecodeWebSocketFrame(std::vector<unsigned char>(buffer.begin(), buffer.begin() + bytesRead));

            // Check for close frame (opcode 0x8) - DecodeWebSocketFrame needs to handle this
            // if (isCloseFrame(buffer, bytesRead)) { // You need to implement isCloseFrame check
            //     std::cout << "[WebSocket] Client " << clientSocket << " sent close frame." << std::endl;
            //     // Optionally send a close frame back
            //     break;
            // }


            if (!message.empty() && messageHandler) {
                try {
                     messageHandler(clientSocket, message);
                } catch (const std::exception& e) {
                     std::cerr << "[WebSocket] Exception in message handler for client " << clientSocket << ": " << e.what() << std::endl;
                }
            } else if (message.empty()) {
                 // This might happen for control frames (ping, pong, close) if Decode doesn't handle them
                 std::cout << "[WebSocket] Client " << clientSocket << " sent empty or non-text/binary frame?" << std::endl;
            }
        }

    } else {
        std::cerr << "[WebSocket] Handshake failed for client " << clientSocket << std::endl;
    }

    // --- Cleanup ---
    std::cout << "[WebSocket] Cleaning up client " << clientSocket << std::endl;
    #if defined(_WIN32) || defined(_WIN64)
        shutdown(clientSocket, SD_BOTH); // Attempt graceful shutdown
        closesocket(clientSocket);
    #else
        shutdown(clientSocket, SHUT_RDWR);
        close(clientSocket);
    #endif

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        if (activeClients.count(clientSocket)) {
            activeClients.erase(clientSocket);
            std::cout << "[WebSocket] Removed client " << clientSocket << " from active list." << std::endl;
        }
    }
     std::cout << "[WebSocket] Thread finished for client " << clientSocket << std::endl;
}

std::string WebSocketServer::DecodeWebSocketFrame(const std::vector<unsigned char>& buffer) {
    // ... Decode 函数的实现 (保持不变) ...
     if (buffer.size() < 2) { // Need at least 2 bytes for basic info
        return "";
    }

    // bool fin = (buffer[0] & 0x80) != 0; // Final frame bit
    unsigned char opcode = buffer[0] & 0x0F; // Opcode

    // --- Handle only text frames (opcode 0x1) for simplicity ---
    if (opcode != 0x1) {
         std::cout << "[WebSocket] Decode: Received non-text frame (opcode " << (int)opcode << "). Ignoring." << std::endl;
        // You should handle close frames (0x8), ping (0x9), pong (0xA) properly
        return "";
    }

    bool masked = (buffer[1] & 0x80) != 0;
    unsigned long long payloadLength = buffer[1] & 0x7F;

    size_t payloadOffset = 2; // Starting position of potential extended length or mask

    if (payloadLength == 126) {
        if (buffer.size() < 4) return ""; // Need 2 more bytes for length
        payloadLength = ((unsigned long long)buffer[2] << 8) | buffer[3];
        payloadOffset = 4;
    } else if (payloadLength == 127) {
        if (buffer.size() < 10) return ""; // Need 8 more bytes for length
        payloadLength = 0;
        for(int i=0; i<8; ++i) {
             payloadLength = (payloadLength << 8) | buffer[payloadOffset + i];
        }
        payloadOffset = 10;
    }

    unsigned char mask[4] = {0};
    if (masked) {
        if (buffer.size() < payloadOffset + 4) return ""; // Need 4 more bytes for mask
        for (int i = 0; i < 4; i++) {
            mask[i] = buffer[payloadOffset + i];
        }
        payloadOffset += 4;
    } else {
         // According to RFC 6455, client-to-server frames MUST be masked
         std::cerr << "[WebSocket] Decode Error: Received unmasked frame from client." << std::endl;
         return ""; // Reject unmasked frames from client
    }

    // Check if buffer contains enough data for the payload
    if (buffer.size() < payloadOffset + payloadLength) {
         std::cout << "[WebSocket] Decode: Buffer too small for payload (Got "
                   << buffer.size() << ", need " << payloadOffset + payloadLength << ")" << std::endl;
        return ""; // Incomplete frame data
    }


    std::string message;
    message.reserve(payloadLength);
    for (size_t i = 0; i < payloadLength; ++i) {
        message += (char)(buffer[payloadOffset + i] ^ mask[i % 4]);
    }

    // Very basic UTF-8 validation (optional, but good practice)
    // This is a very simple check, real validation is more complex.
    // for(size_t i = 0; i < message.length(); ++i) {
    //     // Check for invalid sequences if needed
    // }


    return message;
}

std::vector<unsigned char> WebSocketServer::EncodeWebSocketFrame(const std::string& message) {
    // ... Encode 函数的实现 (保持不变) ...
     std::vector<unsigned char> frame;

    frame.push_back(0x81); // FIN + Text Opcode

    size_t length = message.length();
    if (length <= 125) {
        frame.push_back((unsigned char)length); // Mask bit is 0
    } else if (length <= 65535) {
        frame.push_back(126); // Mask bit is 0
        frame.push_back((length >> 8) & 0xFF);
        frame.push_back(length & 0xFF);
    } else {
        frame.push_back(127); // Mask bit is 0
        // Encode 64-bit length (Big Endian)
        for (int i = 7; i >= 0; i--) {
            frame.push_back((length >> (i * 8)) & 0xFF);
        }
    }

    // Add payload data (no mask needed for server-to-client)
    frame.insert(frame.end(), message.begin(), message.end());

    return frame;
}

bool WebSocketServer::SendToClient(int clientId, const std::string& message) {
     // ... SendToClient 函数的实现 (保持不变) ...
     std::lock_guard<std::mutex> lock(clientsMutex); // Lock is needed here
    if (activeClients.find(clientId) == activeClients.end() || !activeClients[clientId]) {
        std::cerr << "[WebSocket] SendToClient: Client " << clientId << " not found or inactive." << std::endl;
        return false;
    }

    std::vector<unsigned char> frame = EncodeWebSocketFrame(message);
     int bytesSent = send(clientId, (const char*)frame.data(), frame.size(), 0);
     if (bytesSent < (int)frame.size()) {
          std::cerr << "[WebSocket] SendToClient: Failed to send complete message to client " << clientId
                    << " (Sent " << bytesSent << "/" << frame.size() << " bytes)." << std::endl;
          // Consider disconnecting the client here if sends fail repeatedly
          return false;
     }
     return true;
}

void WebSocketServer::Broadcast(const std::string& message) {
     // ... Broadcast 函数的实现 (保持不变) ...
    std::vector<int> clientIdsToSend;
    { // Lock scope
         std::lock_guard<std::mutex> lock(clientsMutex);
         for (auto const& [clientId, isActive] : activeClients) {
             if (isActive) {
                 clientIdsToSend.push_back(clientId);
             }
         }
    } // Unlock mutex

    if (!clientIdsToSend.empty()) {
         std::vector<unsigned char> frame = EncodeWebSocketFrame(message);
         if (frame.empty()) return; // Don't broadcast empty frames

         for (int clientId : clientIdsToSend) {
             // Need to re-lock briefly or handle send errors carefully
             // Simple approach: try sending, log errors
             int bytesSent = send(clientId, (const char*)frame.data(), frame.size(), 0);
              if (bytesSent < (int)frame.size()) {
                  std::cerr << "[WebSocket] Broadcast: Failed to send complete message to client " << clientId
                            << " (Sent " << bytesSent << "/" << frame.size() << " bytes)." << std::endl;
                  // Optionally mark client for removal or disconnect here
              }
         }
    }
}


// --- WebSocketServer.h ---
// 确保 WebSocketServer.h 文件有 PerformHandshake 的声明
// class WebSocketServer {
// private:
//     // ... 其他成员 ...
//     bool PerformHandshake(int clientSocket); // <--- 需要这个声明
// };