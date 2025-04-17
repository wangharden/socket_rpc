#include "GobangGame.h"
#include <chrono>
#include <thread>
void GobangGame::Init_Gobang()
{
	m_pieces.resize(15);
	for (auto& row : m_pieces) {
		row.resize(15);
		for (auto& k : row) k = 2;
	}
	m_expClient = m_client0;
	m_pServer->RegisterFunction(m_client0, std::bind(&GobangGame::GobangGame_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
	m_pServer->RegisterFunction(m_client1, std::bind(&GobangGame::GobangGame_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
	m_lastPi = m_lastPj = -1;
	m_winOne = -1;
	m_bInTakeBackProcess = false;
	m_lHistory.clear();
	m_secPassed = 0;
	m_quit0 = false;
	m_quit1 = false;
}
void GobangGame::Init()
{
	SOCKET c;
	if (m_client0 == SOCKET_ERROR)
	{
		while ((c = m_pServer->Accept()) != SOCKET_ERROR)
		{
			m_client0 = c;
			printf("Get a player, socket: %d\n",(int)m_client0);
			break;
		}
	}
	if (m_client1 == SOCKET_ERROR)
	{
		while ((c = m_pServer->Accept()) != SOCKET_ERROR)
		{
			m_client1 = c;
			printf("Get a player, socket: %d\n", (int)m_client1);
			break;
		}
	}
	this->Init_Gobang();
	m_bReadyTick = true;
}

void GobangGame::DeInit()
{
	
}

void GobangGame::Tick()
{
	if (!this->IsBothReady()) return;
	if (!m_bGameStart)
	{
		Init_Gobang();
		DispatchCommand(std::string("GAME_START"));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		m_tpPiece = std::chrono::steady_clock::now();
		this->NotifyYouTurn(m_expClient);
		m_pServer->SendParams(m_client0, std::string("PIECE_COLOR"), 0);		// 0 for black
		m_pServer->SendParams(m_client1, std::string("PIECE_COLOR"), 1);		// 1 for white
		m_bGameStart = true;
	}
	int w = this->CheckWhoWins();
	if (w != -1)
	{
		int b0Win = w == 0;
		int b1Win = w == 1;
		m_pServer->SendParams(m_client0, std::string("ShowWinLose"), b0Win);
		m_pServer->SendParams(m_client1, std::string("ShowWinLose"), b1Win);
		m_bGameStart = false;
		m_ready0 = false;
		m_ready1 = false;
		DispatchCommand(std::string("GAME_OVER"));
		return;
	}
	auto curTp = std::chrono::steady_clock::now();
	std::chrono::seconds dt = std::chrono::duration_cast<std::chrono::seconds>(curTp - m_tpPiece);
	int Dt = (int)dt.count();
	if (Dt - m_secPassed == 1)
	{
		m_secPassed = Dt;
		this->NotifyYouTurn(m_expClient);
	}
	if (Dt >= m_secInterval)
	{
		m_winOne = m_expClient == m_client0 ? 1 : 0;
	}
}

bool GobangGame::IsBothReady() const
{
	return m_ready0 && m_ready1;
}

bool GobangGame::IsEmptySpot(int i, int j) const
{
	return m_pieces[i][j] == 2;
}

bool GobangGame::IsClientPermitPiece(SOCKET s) const
{
	return !m_bInTakeBackProcess && m_expClient == s;
}

int GobangGame::CheckWhoWins() const
{
	if (m_winOne != -1) return m_winOne;
	if (m_lastPi == -1 || m_lastPj == -1) return -1;
	BYTE pieceType = m_pieces[m_lastPi][m_lastPj];
	//check col wise
	int js = m_lastPj, je = m_lastPj;
	while (js - 1 != -1 && m_pieces[m_lastPi][js - 1] == pieceType) --js;
	while (je + 1 != 15 && m_pieces[m_lastPi][je + 1] == pieceType) ++je;
	if (je - js >= 4) return pieceType;
	//check row wise
	int is = m_lastPi, ie = m_lastPi;
	while (is - 1 != -1 && m_pieces[is - 1][m_lastPj] == pieceType) --is;
	while (ie + 1 != 15 && m_pieces[ie + 1][m_lastPj] == pieceType) ++ie;
	if (ie - is >= 4) return pieceType;
	//check \ dir wise
	int ks = m_lastPi, ke = m_lastPj;
	while (ks - 1 != -1 && ke -1 != -1 && m_pieces[ks-1][ke-1] == pieceType) {
		--ks;
		--ke;
	}
	int gs = m_lastPi, ge = m_lastPj;
	while (gs + 1 != 15 && ge + 1 != 15 && m_pieces[gs + 1][ge + 1] == pieceType) {
		++gs;
		++ge;
	}
	if (gs - ks >= 4) return pieceType;
	// check / dir wise
	ks = m_lastPi, ke = m_lastPj;
	gs = m_lastPi, ge = m_lastPj;
	while (ks - 1 != -1 && ke + 1 != 15 && m_pieces[ks - 1][ke + 1] == pieceType) {
		--ks;
		++ke;
	}
	while (gs + 1 != 15 && ge - 1 != -1 && m_pieces[gs + 1][ge - 1] == pieceType) {
		++gs;
		--ge;
	}
	if (gs - ks >= 4) return pieceType;
	return -1;
}

void GobangGame::TakeOver(SOCKET s0, SOCKET s1)
{
	if (m_pServer)
	{
		m_client0 = s0;
		m_client1 = s1;
		m_pServer->TakeOver(s0);
		m_pServer->TakeOver(s1);
	}
}

void GobangGame::GobangGame_HandleRawData(SOCKET s, const std::string& data)
{
	printf("GobangGame_HandleRawData data length: %d from socket %lld\n", (int)data.size(), s);
	std::string first = SockProto::GetHeadOne(data);
	std::string funcname;
	SockProto::Decode(first, funcname);
	std::string args = data;
	SockProto::RemoveHeadOne(args);
	auto it = m_mapFunc.find(funcname);
	if (m_mapFunc.find(funcname) != m_mapFunc.end()) 
	{
		printf("GobangGame call function: %s\n",funcname.c_str());
		auto& func = m_mapFunc[funcname];
		func(args, s);
	}
	else
	{
		printf("GobangGame has no function named: %s\n",funcname.c_str());
	}
}

bool GobangGame::NeedToBind() const
{
	return m_client0 == SOCKET_ERROR || m_client1 == SOCKET_ERROR;
}

void GobangGame::PieceOn(std::string args, SOCKET s)
{
	int i, j;
	if (!this->IsClientPermitPiece(s)) return;
	SockProto::Decode(args, i, j);
	if (!this->IsEmptySpot(i, j))
		return;
	if (m_client0 == s)
	{
		this->DispatchCommand(std::string("Create_Piece"), i, j, 0);
		m_pieces[i][j] = 0;
		m_expClient = m_client1;
		m_lHistory.push_back({ i,j,0 });
	}
	else if (m_client1 == s)
	{
		this->DispatchCommand(std::string("Create_Piece"), i, j, 1);
		m_pieces[i][j] = 1;
		m_expClient = m_client0;
		m_lHistory.push_back({ i,j,1 });
	}
	m_secPassed = 0;
	m_tpPiece = std::chrono::steady_clock::now();
	this->NotifyYouTurn(m_expClient);
	m_lastPi = i;
	m_lastPj = j;
}

void GobangGame::QuitGame(std::string args, SOCKET s)
{
	m_pServer->CloseConnection(s);
	this->DispatchCommand("Player_Disconnect");
	if (m_client0 == s)
	{
		m_quit0 = true;
	}
	if (m_client1 == s)
	{
		m_quit1 = true;
	}
    m_bCloseGame = m_quit0 && m_quit1;
}
void GobangGame::GiveUp(std::string args, SOCKET s)
{
	if (m_client0 == s)
	{
		m_pServer->SendParams(m_client1, std::string("Give_Up"));
		m_winOne = 1;
	}
	else if (m_client1 == s)
	{
		m_pServer->SendParams(m_client0, std::string("Give_Up"));
		m_winOne = 0;
	}
	else
	{
		return;
	}
	
}

void GobangGame::TakeBack(std::string args, SOCKET s)
{
	if (m_lHistory.size() < 3) return;
	if (!this->IsClientPermitPiece(s)) return;
	if (s == m_client0)
	{// request client 1
		m_pServer->SendParams(m_client1, std::string("TakeBackRequest"));
	}
	else if (s == m_client1)
	{// request client 0
		m_pServer->SendParams(m_client0, std::string("TakeBackRequest"));
	}
	m_bInTakeBackProcess = true;
}

void GobangGame::TakeBackResponse(std::string args, SOCKET s)
{
	int bAgree = 0;
	SockProto::Decode(args, bAgree);
	if (bAgree == 1)
	{
		m_pServer->SendParams(m_expClient, std::string("Agree_TackBack"));
		Piece p0 = m_lHistory.back();
		this->DispatchCommand(std::string("Delete_Piece"), p0.i, p0.j);
		m_lHistory.pop_back();
		m_pieces[p0.i][p0.j] = 2;
		p0 = m_lHistory.back();
		this->DispatchCommand(std::string("Delete_Piece"), p0.i, p0.j);
		m_lHistory.pop_back();
		m_pieces[p0.i][p0.j] = 2;
		p0 = m_lHistory.back();
		m_lastPi = p0.i;
		m_lastPj = p0.j;
		this->DispatchCommand(std::string("Mark_Last_Piece"), p0.i, p0.j);
	}
	else
	{
		m_pServer->SendParams(m_expClient, std::string("Disagree_TackBack"));
	}
	m_bInTakeBackProcess = false;
}

void GobangGame::READY(std::string args, SOCKET s)
{
	int state = 0;
	SockProto::Decode(args, state);
	if (s == m_client0)
	{
		m_ready0 = state;
		m_pServer->SendParams(m_client1, "OTHER_PREPARE", state);
	}
	if (s == m_client1)
	{
		m_ready1 = state;
		m_pServer->SendParams(m_client0, "OTHER_PREPARE", state);
	}
}

