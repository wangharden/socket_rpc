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
//GAME_START ��Ϸ��ʼ
//GAME_OVER ��Ϸ����
//DISPATCH_CARDS չʾ����
//REVEAL_LORD_CARDS չʾ������
//RECEIVE_MY_CALL �е����׶Σ�ʣ��ʱ�䣩
//TIMEOUT_MY_CALL ����е����׶�ʱ��
//RECEIVE_MY_TURN ����/���ƽ׶� (ʣ��ʱ��)
//RECEIVE_MY_DISCARD ���ƽ׶�
//DISCARD_REJECT �ܾ����ƣ������Ϲ���
//TIMEOUT_MY_TURN ������ƽ׶�
//SHOW_DISCARD չʾ����string, �ϼ�/��/�¼� -1/0/1
//RECOGNIFY_DISCARD ���Ƴɹ� ���ƣ�
//ShowWinLose չʾʧ��/ʤ��

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