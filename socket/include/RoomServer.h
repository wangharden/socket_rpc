#pragma once
#include "Utility.h"

//ALREADY_SIGN_UP ֪ͨ�ͻ����Ѿ�ע��
//ALREADY_SIGN_IN ֪ͨ�ͻ����Ѿ���¼
//SIGN_UP_FIRST ֪ͨ�ͻ�����ע��
//SIGN_IN_FIRST ֪ͨ�ͻ����ȵ�¼
//SIGN_IN_SUCCESS ��¼�ɹ�
//SIGN_UP_FAILED ע��ʧ��
//SIGN_UP_SUCCESS ע��ɹ�
//JOIN_SUCCESS ���뷿��ɹ�
//CREATE_ROOM_SUCCESS ��������ɹ�
//CREATE_ROOM_FAILED ��������ʧ��
//ROOM_FULL ������Ա
//ROOM_NONE ���䲻����
//ALREADY_INROOM �Ѿ��ڷ�����
//NOTIFY_READY ֪ͨ�ͻ���׼��״̬ 0/1
//OTHER_JOIN_ROOM �����˼��뷿�� ����
//OTHER_LEAVE_ROOM �������뿪���� ����
//OTHER_READY_STATE ������׼��״̬ ����
//ROOM_LAUNCH ����ƥ��ɹ�����
//REQUEST_ROOM_INFO ���󷿼�״̬��Ϣ

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