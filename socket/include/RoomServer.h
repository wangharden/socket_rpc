#pragma once
#include "Utility.h"

//ALREADY_SIGN_UP 通知客户端已经注册
//ALREADY_SIGN_IN 通知客户端已经登录
//SIGN_UP_FIRST 通知客户端先注册
//SIGN_IN_FIRST 通知客户端先登录
//SIGN_IN_SUCCESS 登录成功
//SIGN_UP_FAILED 注册失败
//SIGN_UP_SUCCESS 注册成功
//JOIN_SUCCESS 加入房间成功
//CREATE_ROOM_SUCCESS 创建房间成功
//CREATE_ROOM_FAILED 创建房间失败
//ROOM_FULL 房间满员
//ROOM_NONE 房间不存在
//ALREADY_INROOM 已经在房间中
//NOTIFY_READY 通知客户端准备状态 0/1
//OTHER_JOIN_ROOM 其他人加入房间 名字
//OTHER_LEAVE_ROOM 其他人离开房间 名字
//OTHER_READY_STATE 其他人准备状态 名字
//ROOM_LAUNCH 房间匹配成功启动
//REQUEST_ROOM_INFO 请求房间状态信息

class RoomServer : public BlockedServer
{
    typedef int UID;

    struct Player
    {
        UID uid = -1;
        std::string name = "";
        std::string password = "";
    };

    struct LobbyPlayer : public Player
    {
        SOCKET socket = SOCKET_ERROR;
        std::string roomID = "";
        int state = 0;
    };

    struct Room
    {
        std::string roomID = "";
        std::vector<UID> members;
        int pid = 0;            //non zero indicate in running game
        std::string roomName = "";
        int gameType = 0;
        std::string GetRoomInfo()const;
    };

    std::map<UID, Player> m_mapSignUpPlayer;              //already registered player

    std::map<UID, LobbyPlayer> m_mapInGamePlayer;
    std::map<UID, LobbyPlayer> m_mapInLobbyPlayer;          //not in game player

    std::map<std::string, Room> m_mapInGameRoom;
    std::map<std::string, Room> m_mapInLobbyRoom;           //not in game room
protected:
    bool CheckPlayerOffline(UID id)const;
    void InitPlayerData();
    bool Record_SignUp(UID id, std::string name);
    UID GenerateValidID()const;
    bool IsUserSignedUp(std::string name)const;
    bool IsUserSignedIn(UID id)const;
    UID FindSignedUpUID(std::string name)const;
    UID FindSignedInUID(SOCKET s)const;
private:
    std::map<std::string, std::function<void(std::string, SOCKET)>> m_mapFunc;
protected:
    void RemovePlayerFromRoom(const LobbyPlayer& player);
    void Notify_JoinRoom(SOCKET s, std::string roomName);          //notify other
    void Notify_LeaveRoom(SOCKET s, std::string roomName);         //notify other
    void Notify_ReadyState(SOCKET s, std::string roomName, int state);  //notify other
    std::string GetRoomInfo()const;
public:
    void SIGN_UP(std::string data, SOCKET s);
    void SIGN_IN(std::string data, SOCKET s);
    void ROOM(std::string data, SOCKET s);
    void CREATE_ROOM(std::string data, SOCKET s);
    void READY(std::string data, SOCKET s);
    void QUIT_LOBBY(std::string data, SOCKET s);
    void REQUEST_ROOM_INFO(std::string data, SOCKET s);
public:
    void RoomServer_HandleRawData(SOCKET s, const std::string& data);
public:
    std::string PlayerDB;
    RoomServer(std::string playerDB = "player_data.txt") : PlayerDB(playerDB) {
        SetSocketNonBlocking(m_sock);
        m_mapFunc["ROOM"] = std::bind(&RoomServer::ROOM, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["CREATE_ROOM"] = std::bind(&RoomServer::CREATE_ROOM, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["REQUEST_ROOM_INFO"] = std::bind(&RoomServer::REQUEST_ROOM_INFO, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["READY"] = std::bind(&RoomServer::READY, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["QUIT_LOBBY"] = std::bind(&RoomServer::QUIT_LOBBY, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["SIGN_UP"] = std::bind(&RoomServer::SIGN_UP, this, std::placeholders::_1, std::placeholders::_2);
        m_mapFunc["SIGN_IN"] = std::bind(&RoomServer::SIGN_IN, this, std::placeholders::_1, std::placeholders::_2);
        InitPlayerData();
    }
    virtual void Tick();
    bool GetReadyRoom(std::vector<SOCKET>& members, std::string& room);
    void MarkRoomLaunch(const std::string& room, bool bLaunch, int pid);
    virtual bool ShouldReceiveData(SOCKET s)const;
};