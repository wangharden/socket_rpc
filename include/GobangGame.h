#pragma once
#include "Game.h"
#include <map>
#include <vector>
#include <list>
#include <chrono>
//GAME_START
//GAME_OVER
//PIECE_COLOR 0: black 1: white
//CREATE_PIECE i, j, 0: black 1: white
using std::vector;
class GobangGame :public Game
{
	SOCKET m_client0 = SOCKET_ERROR;
	SOCKET m_client1 = SOCKET_ERROR;
	SOCKET m_expClient = SOCKET_ERROR;
	bool m_ready0 = false;
	bool m_ready1 = false;
	bool m_quit0 = false;
	bool m_quit1 = false;
	bool m_bGameStart = false;
	vector<vector<BYTE>> m_pieces;
	int m_lastPi = -1;
	int m_lastPj = -1;
	
	int m_winOne = -1;
	bool m_bInTakeBackProcess = false;
	struct Piece
	{
		int i;
		int j;
		int client;
	};
	std::list<Piece> m_lHistory;
	std::chrono::time_point<std::chrono::steady_clock> m_tpPiece;
	int m_secPassed = 0;
	int m_secInterval = 30;
protected:
	void Init_Gobang();
	virtual void Init();
	virtual void DeInit();
	virtual void Tick();
	bool IsBothReady()const;
	bool IsEmptySpot(int i, int j)const;
	bool IsClientPermitPiece(SOCKET s)const;
	int CheckWhoWins()const;
	template<class ...Args>
	void DispatchCommand(Args ...args)
	{
		m_pServer->SendParams(m_client0, args...);
		m_pServer->SendParams(m_client1, args...);
	}
	void NotifyYouTurn(SOCKET s){
		std::string cmds = "Receive_MyTurn";
		std::string cmds1 = "Receive_OtherTurn";
		int leftTime = m_secInterval - m_secPassed;
		if (s == m_client0){
			m_pServer->SendParams(s, cmds, leftTime);
			m_pServer->SendParams(m_client1, cmds1, leftTime);
		}
		else
		{
			m_pServer->SendParams(s, cmds, leftTime);
			m_pServer->SendParams(m_client0, cmds1, leftTime);
		}
	}
	void TakeOver(SOCKET s0, SOCKET s1);
public:
	void GobangGame_HandleRawData(SOCKET s, const std::string& data);
	virtual bool NeedToBind()const;
public:
	std::map<std::string, std::function<void(std::string, SOCKET)>> m_mapFunc;
	GobangGame() :Game()
	{
		m_mapFunc["PieceOn"] = std::bind(&GobangGame::PieceOn, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["QuitGame"] = std::bind(&GobangGame::QuitGame, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["GiveUp"] = std::bind(&GobangGame::GiveUp, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["TakeBack"] = std::bind(&GobangGame::TakeBack, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["TakeBackResponse"] = std::bind(&GobangGame::TakeBackResponse, this, std::placeholders::_1, std::placeholders::_2);
		m_mapFunc["READY"] = std::bind(&GobangGame::READY, this, std::placeholders::_1, std::placeholders::_2);
	}
	GobangGame(SOCKET s0, SOCKET s1) : GobangGame()
	{
		this->TakeOver(s0, s1);
	}
	//registered function
	void PieceOn(std::string args, SOCKET s);
	void QuitGame(std::string args, SOCKET s);
	void GiveUp(std::string args, SOCKET s);
	void TakeBack(std::string args, SOCKET s);
	void TakeBackResponse(std::string args, SOCKET s);
	void READY(std::string args, SOCKET s);
};
