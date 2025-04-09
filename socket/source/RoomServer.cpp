#include "RoomServer.h"
#include "Game.h"
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;
void RoomServer::RoomServer_HandleRawData(SOCKET s, const std::string& data)
{
    std::string first = SockProto::GetHeadOne(data);
    std::string funcname;
    SockProto::Decode(first, funcname);
    std::string args = data;
    SockProto::RemoveHeadOne(args);
    auto it = m_mapFunc.find(funcname);
    if (it != m_mapFunc.end())
    {
        printf("Call RoomServer::%s\n", funcname.c_str());
        (*it).second(args, s);
    }
}

void RoomServer::Tick()
{
    BlockedServer::Tick();
    SOCKET s = this->Accept();
    if (s != SOCKET_ERROR)
    {
        printf("RoomServer get a connection: %lld\n", s);
        this->RegisterFunction(s, std::bind(&RoomServer::RoomServer_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
    }
    else
    {
        //printf("RoomServer waiting connection\n");
    }

#if defined(ENV_UNIX)
    int status;
    std::map<std::string, int> waitingDelete;
    for (auto& item : m_mapInGameRoom)
    {
        pid_t pid = waitpid(item.second.pid, &status, WNOHANG);
        if (pid > 0)
        {//child process shutdown
            waitingDelete[item.first] = item.second.pid;
        }
    }
    for (auto& v : waitingDelete)
    {
        MarkRoomLaunch(v.first, false, v.second);
    }
#endif
    std::vector<UID> readyToRemove;
    for (auto& v : m_mapInLobbyPlayer)
    {
        int sockID = this->FindSocketID(v.second.socket);
        if (sockID != -1)
        {
            if (m_acceptsock[sockID].state == CLOSED)
            {
                printf("Client %s dismiss, remove from lobby\n", v.second.name.c_str());
                readyToRemove.push_back(v.first);
            }
        }
    }
    for (auto id : readyToRemove)
    {
        LobbyPlayer p = m_mapInLobbyPlayer[id];
        m_mapInLobbyPlayer.erase(id);
        if (p.roomID != "")
        {
            auto rit = m_mapInLobbyRoom.find(p.roomID);
            if (rit != m_mapInLobbyRoom.end())
            {
                auto& mem = (*rit).second.members;
                auto it = std::find(mem.begin(), mem.end(), id);
                if (it != mem.end())
                {
                    printf("User %d dismiss, remove from lobby room\n", id);
                    mem.erase(it);
                }
                if (mem.size() == 0)
                {
                    printf("Room %s is empty, destroy and remove from lobby\n", p.roomID.c_str());
                    m_mapInLobbyRoom.erase(p.roomID);
                }
            }
        }
    }
}

std::string RoomServer::GetRoomInfo() const
{
    std::string infos = "";
    for (auto& v : m_mapInLobbyRoom)
    {
        infos += "{";
        infos += v.second.GetRoomInfo();
        infos += "}";
    }
    for (auto& v : m_mapInGameRoom)
    {
        infos += "{";
        infos += v.second.GetRoomInfo();
        infos += "}";
    }
    return infos;
}

bool RoomServer::GetReadyRoom(std::vector<SOCKET>& members, std::string& room)
{
    for (auto& v : m_mapInLobbyRoom)
    {
        bool ready = true;
        int capcity = Game::GetGameCapcity(v.second.gameType);
        if (v.second.members.size() != capcity)
            continue;
        for (auto& p : v.second.members)
        {
            LobbyPlayer& player = m_mapInLobbyPlayer[p];
            if (player.state == 0) {
                ready = false;
                break;
            }
        }
        if (ready)
        {
            room = v.first;
            members.clear();
            for (auto& p : v.second.members)
            {
                LobbyPlayer& player = m_mapInLobbyPlayer[p];
                members.push_back(player.socket);
            }
            return true;
        }
    }
    return false;
}

bool RoomServer::CheckPlayerOffline(UID id) const
{
    bool res = true;
    printf("RoomServer::StartCheckPlayerOffline\n");
    auto itInGame = m_mapInGamePlayer.find(id);
    if (itInGame != m_mapInGamePlayer.end())
    {
        printf("RoomServer::CheckPlayerOffline user %d still in game.\n", id);
        res = false;
    }
    auto itInLobby = m_mapInLobbyPlayer.find(id);
    if (itInLobby != m_mapInLobbyPlayer.end())
    {
        printf("RoomServer::CheckPlayerOffline user %d still in lobby.\n", id);
        res = false;
    }
    for (auto& v : m_mapInGameRoom)
    {
        for (auto gid : v.second.members)
        {
            if (id == gid)
            {
                printf("RoomServer::CheckPlayerOffline user %d still in game room %s.\n", id, v.first.c_str());
                res = false;
            }
        }
    }
    for (auto& v : m_mapInLobbyRoom)
    {
        for (auto gid : v.second.members)
        {
            if (id == gid)
            {
                printf("RoomServer::CheckPlayerOffline user %d still in lobby room %s.\n", id, v.first.c_str());
                res = false;
            }
        }
    }
    printf("InGame player:\n");
    for (auto& v : m_mapInGamePlayer)
    {
        printf("%d %s\n", v.first, v.second.name.c_str());
    }
    printf("InLobby player:\n");
    for (auto& v : m_mapInLobbyPlayer)
    {
        printf("%d %s\n", v.first, v.second.name.c_str());
    }

    return res;
}

void RoomServer::InitPlayerData()
{
    if (PlayerDB == "")
        return;
    ifstream fin(PlayerDB);
    if (!fin)
    {
        printf("Open player database %s failed\n", PlayerDB.c_str());
        return;
    }
    UID id;
    std::string name;
    while (!fin.eof())
    {
        fin >> id >> name;
        m_mapSignUpPlayer[id] = Player{ id,name };
    }
    fin.close();
    printf("Sign up infomation:\n");
    for (auto& v : m_mapSignUpPlayer)
    {
        printf("UID: %d, Name: %s\n", v.first, v.second.name.c_str());
    }
}

bool RoomServer::Record_SignUp(UID id, std::string name)
{
    m_mapSignUpPlayer[id] = Player{ id,name };
    using std::ofstream;
    ofstream fout(PlayerDB, std::ios::app);
    if (!fout)
    {
        printf("open player database %s failed\n", PlayerDB.c_str());
        return false;
    }
    fout << id << " " << name << std::endl;
    fout.close();
    return true;
}

RoomServer::UID RoomServer::GenerateValidID() const
{
    int id = 0;

    for (auto& v : m_mapSignUpPlayer) {
        id = v.first > id ? v.first : id;
    }
    id += 1;
    return id;
}

bool RoomServer::IsUserSignedUp(std::string name) const
{
    for (auto& v : m_mapSignUpPlayer)
    {
        if (v.second.name == name) {
            return true;
        }
    }
    return false;
}

//�Ƿ��ڵ�¼״̬����Ϸ��Ҳ���¼״̬
bool RoomServer::IsUserSignedIn(UID id) const
{
    return m_mapInLobbyPlayer.find(id) != m_mapInLobbyPlayer.end() ||
        m_mapInGamePlayer.find(id) != m_mapInGamePlayer.end();
}

RoomServer::UID RoomServer::FindSignedUpUID(std::string name) const
{
    for (auto& v : m_mapSignUpPlayer)
    {
        if (v.second.name == name)
        {
            return v.first;
        }
    }
    return 0;
}

RoomServer::UID RoomServer::FindSignedInUID(SOCKET s) const
{
    UID id = 0;
    for (auto& v : m_mapInLobbyPlayer)
    {
        if (v.second.socket == s)
        {
            return v.first;
        }
    }
    for (auto& v : m_mapInGamePlayer)
    {
        if (v.second.socket == s)
        {
            return v.first;
        }
    }
    return id;
}

void RoomServer::RemovePlayerFromRoom(const LobbyPlayer& player)
{
    if (player.roomID != "")
    {
        Room& R = m_mapInLobbyRoom[player.roomID];
        auto& vec = R.members;
        auto itpos = std::find(vec.begin(), vec.end(), player.uid);
        if (itpos == vec.end())
            return;
        std::string name = m_mapSignUpPlayer[player.uid].name;
        printf("RoomServer::QUIT_LOBBY player %s disconnect\n", name.c_str());
        vec.erase(itpos);
        this->Notify_LeaveRoom(player.socket, player.roomID);
        if (vec.empty())
        {//room is empty, then remove it.
            m_mapInLobbyRoom.erase(player.roomID);
            printf("RoomServer::RemovePlayerFromRoom room %s empty, be removed.\n", player.roomID.c_str());
        }
    }
}

void RoomServer::Notify_JoinRoom(SOCKET s, std::string roomName)
{
    UID id = this->FindSignedInUID(s);
    Player p = m_mapSignUpPlayer[id];
    auto& mem = m_mapInLobbyRoom[roomName];
    for (auto other : mem.members)
    {
        auto& player = m_mapInLobbyPlayer[other];
        if (player.socket != s)
        {
            this->SendParams(player.socket, std::string("OTHER_JOIN_ROOM"), p.name);
        }
    }
}

void RoomServer::Notify_LeaveRoom(SOCKET s, std::string roomName)
{
    UID id = this->FindSignedInUID(s);
    Player p = m_mapSignUpPlayer[id];
    if (m_mapInLobbyRoom.find(roomName) != m_mapInLobbyRoom.end())
    {
        auto& mem = m_mapInLobbyRoom[roomName];
        for (auto other : mem.members)
        {
            if (m_mapInLobbyPlayer.find(other) == m_mapInLobbyPlayer.end())
            {
                printf("RoomServer::Notify_LeaveRoom userid %d not in lobby.\n", other);
                continue;
            }
            auto& player = m_mapInLobbyPlayer[other];
            if (player.socket != s)
            {
                this->SendParams(player.socket, std::string("OTHER_LEAVE_ROOM"), p.name);
            }
        }
    }
    else
    {
        printf("RoomServer::Notify_LeaveRoom room %s dismiss.\n", roomName.c_str());
    }
}

void RoomServer::Notify_ReadyState(SOCKET s, std::string roomName, int state)
{
    UID id = this->FindSignedInUID(s);
    Player p = m_mapSignUpPlayer[id];
    auto& mem = m_mapInLobbyRoom[roomName];
    for (auto other : mem.members)
    {
        auto& player = m_mapInLobbyPlayer[other];
        if (player.socket != s)
        {
            this->SendParams(player.socket, std::string("OTHER_READY_STATE"), state, p.name);
        }
    }
}

void RoomServer::SIGN_UP(std::string data, SOCKET s)
{
    std::string name;
    SockProto::Decode(data, name);
    if (this->IsUserSignedUp(name)) {
        this->SendParams(s, std::string("ALREADY_SIGN_UP"));
        return;
    }
    UID id = GenerateValidID();
    bool res = Record_SignUp(id, name);
    if (res)
    {
        this->SendParams(s, std::string("SIGN_UP_SUCCESS"));
        printf("RoomServer::SIGN_UP %s sign up success\n", name.c_str());
    }
    else
    {
        this->SendParams(s, std::string("SIGN_UP_FAILED"));
    }
}

void RoomServer::SIGN_IN(std::string data, SOCKET s)
{
    std::string name;
    SockProto::Decode(data, name);
    if (this->IsUserSignedUp(name))
    {
        UID id = this->FindSignedUpUID(name);
        if (this->IsUserSignedIn(id))
        {
            this->SendParams(s, std::string("ALREADY_SIGN_IN"));
            return;
        }
        LobbyPlayer player;
        player.uid = id;
        player.name = name;
        player.socket = s;
        m_mapInLobbyPlayer[id] = player;
        this->SendParams(s, std::string("SIGN_IN_SUCCESS"));
        printf("RoomServer::SIGN_IN %s signed in.\n", name.c_str());
    }
    else
    {
        this->SendParams(s, std::string("SIGN_UP_FIRST"));
        return;
    }
}
void RoomServer::CREATE_ROOM(std::string data, SOCKET s)
{
    UID id = this->FindSignedInUID(s);
    if (id == 0)
    {
        this->SendParams(s, std::string("SIGN_IN_FIRST"));
        return;
    }
    std::string args = data;
    std::string roomNumber;
    std::string roomName;
    int gameType;
    SockProto::Decode(args, roomNumber, roomName, gameType);
    LobbyPlayer& player = m_mapInLobbyPlayer[id];

    if (m_mapInLobbyRoom.find(roomNumber) == m_mapInLobbyRoom.end())
    {
        if (player.roomID != "")
        {
            this->RemovePlayerFromRoom(player);
        }
        Room r;
        r.roomID = roomNumber;
        r.members = std::vector<UID>();
        r.roomName = roomName;
        r.gameType = gameType;
        m_mapInLobbyRoom[roomNumber] = r;
        player.roomID = roomNumber;
        this->SendParams(s, std::string("CREATE_ROOM_SUCCESS"));
        Room& room = m_mapInLobbyRoom[roomNumber];
        room.members.push_back(id);
        std::string roomInfos = this->GetRoomInfo();
        this->SendParams(s, "UPDATE_ROOMINFO", roomInfos);
    }
    else
    {
        this->SendParams(s, std::string("CREATE_ROOM_FAILED"));
    }

}
void RoomServer::ROOM(std::string data, SOCKET s)
{
    UID id = this->FindSignedInUID(s);
    if (id == 0)
    {
        this->SendParams(s, std::string("SIGN_IN_FIRST"));
        return;
    }
    std::string args = data;
    std::string roomNumber;
    std::string roomName;
    //SockProto::Decode(args, roomNumber);
    SockProto::Decode(args, roomNumber, roomName);
    LobbyPlayer& player = m_mapInLobbyPlayer[id];


    if (m_mapInLobbyRoom.find(roomNumber) == m_mapInLobbyRoom.end())
    {
        this->SendParams(s, "ROOM_NONE");
        return;
    }
    Room& room = m_mapInLobbyRoom[roomNumber];
    auto& roomMember = room.members;
    int capcity = Game::GetGameCapcity(room.gameType);
    if (std::find(roomMember.begin(), roomMember.end(), id) == roomMember.end() && roomMember.size() < capcity) {

        //not in this room
        size_t n = roomMember.size();
        roomMember.push_back(id);
        Notify_JoinRoom(s, roomNumber);
        if (n != 0)
        {//already has roomate
            this->SendParams(s, std::string("JOIN_SUCCESS"));
        }
        player.roomID = roomNumber;
    }
    else if (roomMember.size() >= capcity)
    {
        this->SendParams(s, std::string("ROOM_FULL"));
    }
    else
    {
        this->SendParams(s, std::string("ALREADY_INROOM"));
    }
}

void RoomServer::READY(std::string data, SOCKET s)
{
    UID id = this->FindSignedInUID(s);
    if (id == 0)
    {
        this->SendParams(s, std::string("SIGN_IN_FIRST"));
        return;
    }
    LobbyPlayer& player = m_mapInLobbyPlayer[id];
    if (player.roomID == "")
    {
        this->SendParams(s, std::string("ENTER_ROOM_FIRST"));
        return;
    }
    int state;
    SockProto::Decode(data, state);
    player.state = state;
    this->SendParams(s, std::string("NOTIFY_READY"), state);
    std::string RoomName = player.roomID;
    this->Notify_ReadyState(s, RoomName, state);
}

void RoomServer::QUIT_LOBBY(std::string data, SOCKET s)
{
    //player leave lobby, should check room.
    UID id = this->FindSignedInUID(s);
    if (m_mapInLobbyPlayer.find(id) != m_mapInLobbyPlayer.end())
    {//in lobby
        LobbyPlayer& player = m_mapInLobbyPlayer[id];
        this->RemovePlayerFromRoom(player);
        m_mapInLobbyPlayer.erase(id);
    }
    else
    {//in game
        m_mapInGamePlayer.erase(id);
    }
    CheckPlayerOffline(id);
}

void RoomServer::REQUEST_ROOM_INFO(std::string data, SOCKET s)
{
    std::string roomInfos = this->GetRoomInfo();
    if (!roomInfos.empty()) {
        this->SendParams(s, std::string("UPDATE_ROOMINFO"), roomInfos);
    }
}

void RoomServer::MarkRoomLaunch(const std::string& room, bool bLaunch, int pid)
{
    if (bLaunch)
    {
        Room R = m_mapInLobbyRoom[room];
        m_mapInLobbyRoom.erase(room);
        R.pid = pid;
        m_mapInGameRoom[room] = R;
        for (auto id : R.members)
        {
            LobbyPlayer p = m_mapInLobbyPlayer[id];
            m_mapInLobbyPlayer.erase(id);
            m_mapInGamePlayer[id] = p;
            this->SendParams(p.socket, std::string("ROOM_LAUNCH"));
        }
        printf("RoomServer::MarkRoomLaunch room %s start game.\n", room.c_str());
    }
    else
    {
        Room R = m_mapInGameRoom[room];
        m_mapInGameRoom.erase(room);
        R.pid = 0;
        m_mapInLobbyRoom[room] = R;

        for (auto id : R.members)
        {
            LobbyPlayer p = m_mapInGamePlayer[id];
            m_mapInGamePlayer.erase(id);
            p.state = 0;
            m_mapInLobbyPlayer[id] = p;
            int num = this->SendParams(p.socket, std::string("NOTIFY_READY"), 0);
            if (num == SOCKET_ERROR)
            {// user already quit lobby
                m_mapInLobbyPlayer.erase(id);
            }
        }
        printf("RoomServer::MarkRoomLaunch room %s gameover.\n", room.c_str());
    }
}

bool RoomServer::ShouldReceiveData(SOCKET s)const
{
    for (auto& v : m_mapInGamePlayer)
    {
        if (v.second.socket == s)
        {
            return false;
        }
    }
    return true;
}

std::string RoomServer::Room::GetRoomInfo() const
{
    std::string info = "";
    info += roomID + "\n";
    info += roomName + "\n";
    std::string gameName = Game::GetGameName(gameType);
    info += gameName + "\n";
    std::string status = pid == 0 ? "Not Ready" : "In Game";
    info += status + "\n";
    int capcity = Game::GetGameCapcity(gameType);
    int num = (int)members.size();
    info += std::to_string(num) + "/" + std::to_string(capcity);
    return info;
}