#pragma once
#include "Game.h"
#include <vector>
#include "Poker.h"
#include <chrono>

using std::vector;
using namespace std::chrono;
struct LandlordPlayer
{
	SOCKET socket = SOCKET_ERROR;
	bool ready = false;
	bool quit = false;
	Holds hold;
	bool bLord = false;
};
//1: dispatch cards
//2: call lord!!!
//3: reveal lord cards
//4: start from lord
//GAME_START 游戏开始
//GAME_OVER 游戏结束
//DISPATCH_CARDS 展示卡牌
//REVEAL_LORD_CARDS 展示地主牌
//RECEIVE_MY_CALL 叫地主阶段（剩余时间）
//TIMEOUT_MY_CALL 错过叫地主阶段时间
//RECEIVE_MY_TURN 出牌/过牌阶段 (剩余时间)
//RECEIVE_MY_DISCARD 出牌阶段
//DISCARD_REJECT 拒绝出牌，不符合规则
//TIMEOUT_MY_TURN 错过出牌阶段
//SHOW_DISCARD 展示出牌string, 上家/我/下家 -1/0/1
//RECOGNIFY_DISCARD 出牌成功 （牌）
//ShowWinLose 展示失败/胜利

class Landlord :public Game
{
	vector<LandlordPlayer> m_client;
	bool m_bGameStart = false;
	bool m_bDispatchFinish = false;
	bool m_bCallLordFinish = false;
	bool m_bRevealCardsFinish = false;
	int m_expClient = -1;
	time_point<steady_clock> m_expTp;
	int m_secLeftCallLord;
	int m_secLeftOutCard;
	const int m_secCallLoardInterval = 10;
	const int m_secOutCardInterval = 20;
	Deck m_deck;
	struct HandInfo {
		Holds hold;
		int clientID;

	};
	vector<HandInfo> m_histroy;
protected:
	void TakeOver(SOCKET s0, SOCKET s1, SOCKET s2);
	virtual void Init();
	virtual void DeInit() {}
	virtual void Tick();
	bool IsAllReady()const;
	void Init_Landlord();
	int CheckWhoWins()const;
	template<class ...Args>
	void DispatchCommand(Args ...args)
	{
		m_pServer->SendParams(m_client[0].socket, args...);
		m_pServer->SendParams(m_client[1].socket, args...);
		m_pServer->SendParams(m_client[2].socket, args...);
	}
	void DispatchCards();
	void CallLord();
	void RevealLordCards();
	int FindPlayer(SOCKET s)const;
	void NotifyYouTurn(SOCKET s);
	void NotifyYouDiscard(SOCKET s);
	void NotifyYouCall(SOCKET s);
	void ShowDiscard(SOCKET s, std::string card)const;
	bool IsPermittedDiscard(const Holds& hold)const;
public:
	void Landlord_HandleRawData(SOCKET s, const std::string& data);
	virtual bool NeedToBind()const;
public:
	std::map<std::string, std::function<void(std::string, SOCKET)>> m_mapFunc;
	Landlord() :Game() {
		m_client.resize(3);
		m_mapFunc["READY"] = std::bind(&Landlord::READY, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["CALL_LORD"] = std::bind(&Landlord::CALL_LORD, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["DISCARD"] = std::bind(&Landlord::DISCARD, this, std::placeholders::_1, std::placeholders::_2);
	}
	Landlord(SOCKET s0, SOCKET s1, SOCKET s2) :Landlord()
	{
		this->TakeOver(s0, s1, s2);
	}
	//registered function
	void READY(std::string args, SOCKET s);
	void CALL_LORD(std::string args, SOCKET s);
	void DISCARD(std::string args, SOCKET s);
};