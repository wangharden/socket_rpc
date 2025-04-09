#include "Landlord.h"
#include <thread>
#include <chrono>
#include <cassert>
void Landlord::TakeOver(SOCKET s0, SOCKET s1, SOCKET s2)
{
	if (m_pServer)
	{
		m_client[0].socket = s0;
		m_client[1].socket = s1;
		m_client[2].socket = s2;
		m_pServer->TakeOver(s0);
		m_pServer->TakeOver(s1);
		m_pServer->TakeOver(s2);
	}
}

void Landlord::Init()
{
	SOCKET c;
	for (int i = 0; i < 3; ++i)
	{
		LandlordPlayer& player = m_client[i];
		if (player.socket == SOCKET_ERROR)
		{
			while ((c = m_pServer->Accept()) != SOCKET_ERROR) {
				player.socket = c;
				printf("Get a player, socket: %d\n", (int)c);
				break;
			}
		}
	}
	this->Init_Landlord();
	m_bReadyTick = true;
}

void Landlord::Tick()
{
	if (!this->IsAllReady()) return;
	if (!m_bGameStart)
	{
		Init_Landlord();
		DispatchCommand(std::string("GAME_START"));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		

		m_bGameStart = true;
	}
	if (!m_bDispatchFinish)
	{
		DispatchCards();
		std::this_thread::sleep_for(std::chrono::seconds(5));
		m_bDispatchFinish = true;
		m_expClient = 0;
		NotifyYouCall(m_client[m_expClient].socket);
	}
	if (!m_bCallLordFinish)
	{
		CallLord();
		return;
	}
	if (!m_bRevealCardsFinish)
	{
		RevealLordCards();
		m_bRevealCardsFinish = true;
		this->NotifyYouDiscard(m_client[m_expClient].socket);
	}

	{
		int id = m_expClient;
		auto dt = duration_cast<seconds>(m_expTp - steady_clock::now());
		if (m_secLeftOutCard > dt.count())
		{
			m_secLeftOutCard = dt.count();
			if (dt.count() <= 0)
			{
				m_pServer->SendParams(m_client[id].socket, "TIMEOUT_MY_TURN");
				m_expClient = (id + 1) % 3;
				this->NotifyYouTurn(m_client[m_expClient].socket);
			}
			else
			{
				m_pServer->SendParams(m_client[id].socket, "RECEIVE_MY_TURN", m_secLeftOutCard);
			}
		}
	}

	int w = this->CheckWhoWins();
	if (w != -1)
	{
		int b0Win = w == 0;
		int b1Win = w == 1;
		int b2Win = w == 2;
		m_pServer->SendParams(m_client[0].socket, std::string("ShowWinLose"), b0Win);
		m_pServer->SendParams(m_client[1].socket, std::string("ShowWinLose"), b1Win);
		m_pServer->SendParams(m_client[2].socket, std::string("ShowWinLose"), b2Win);
		m_bGameStart = false;
		for (int i = 0; i < 3; ++i)
		{
			m_client[i].ready = false;
		}
		this->DispatchCommand("GAME_OVER");
	}

}

bool Landlord::IsAllReady() const
{
	for (int i = 0; i < 3; ++i)
	{
		if (m_client[i].quit)
			return false;
		if (!m_client[i].ready)
			return false;
	}
	return true;
}

void Landlord::Init_Landlord()
{
	m_deck = Deck();
	m_deck.Shuffle();
	m_expClient = -1;
	m_bGameStart = false;
	m_bDispatchFinish = false;
	m_bCallLordFinish = false;
	m_bRevealCardsFinish = false;
	m_pServer->RegisterFunction(m_client[0].socket, std::bind(&Landlord::Landlord_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
	m_pServer->RegisterFunction(m_client[1].socket, std::bind(&Landlord::Landlord_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
	m_pServer->RegisterFunction(m_client[2].socket, std::bind(&Landlord::Landlord_HandleRawData, this, std::placeholders::_1, std::placeholders::_2));
}

int Landlord::CheckWhoWins() const
{
	for (int i = 0; i < 3; ++i)
	{
		if (m_client[i].hold.cards.empty())
		{
			return i;
		}
	}
	return -1;
}

void Landlord::DispatchCards()
{
	for (int i = 0; i < 17; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			m_client[j].hold.Insert(m_deck.PopOne());
		}
	}
	for (int i = 0; i < 3; ++i)
	{
		m_client[i].hold.Sort();
		std::string s = m_client[i].hold.Encode();
		m_pServer->SendParams(m_client[i].socket, std::string("DISPATCH_CARDS"), s);
	}
}

void Landlord::CallLord()
{
	if (m_expClient != -1)
	{
		if (m_client[m_expClient].bLord)
		{
			m_bCallLordFinish = true;
			return;
		}
	}
	int id = m_expClient;
	auto dt = duration_cast<seconds>(m_expTp - steady_clock::now());
	if (m_secLeftCallLord > dt.count())
	{
		m_secLeftCallLord = dt.count();
		if (dt.count() <= 0)
		{
			m_pServer->SendParams(m_client[id].socket, "TIMEOUT_MY_CALL");
			m_expClient = (id + 1) % 3;
			this->NotifyYouCall(m_client[m_expClient].socket);
		}
		else
		{
			m_pServer->SendParams(m_client[id].socket, "RECEIVE_MY_CALL", m_secLeftCallLord);
		}
	}
}

void Landlord::RevealLordCards()
{
	Holds h;
	h.cards = m_deck.cards;
	std::string s = h.Encode();
	DispatchCommand("REVEAL_LORD_CARDS", s);
}

int Landlord::FindPlayer(SOCKET s) const
{
	for (int i = 0; i < 3; ++i) {
		if (s == m_client[i].socket)
			return i;
	}
	return -1;
}

void Landlord::NotifyYouTurn(SOCKET s)
{
	int id = this->FindPlayer(s);
	if (id != -1)
	{
		m_expTp = steady_clock::now() + seconds(m_secOutCardInterval);
		m_secLeftOutCard = m_secOutCardInterval;
		m_pServer->SendParams(s, std::string("RECEIVE_MY_TURN"), m_secLeftOutCard);
	}
}

void Landlord::NotifyYouDiscard(SOCKET s)
{
	int id = FindPlayer(s);
	if (id != -1)
	{
		m_expTp = steady_clock::now() + seconds(m_secOutCardInterval);
		m_secLeftOutCard = m_secOutCardInterval;
		m_pServer->SendParams(s, std::string("RECEIVE_MY_DISCARD"), m_secLeftOutCard);
	}
}

void Landlord::NotifyYouCall(SOCKET s)
{
	m_expTp = steady_clock::now() + seconds(m_secCallLoardInterval);
	m_secLeftCallLord = m_secCallLoardInterval;
	m_pServer->SendParams(s, "RECEIVE_MY_CALL", m_secLeftCallLord);
}

void Landlord::ShowDiscard(SOCKET s, std::string card) const
{
	int id = FindPlayer(s);
	if (id != -1)
	{
		m_pServer->SendParams(s, "SHOW_DISCARD", card, 0);
		id = (id + 1) % 3;
		m_pServer->SendParams(m_client[id].socket, "SHOW_DISCARD", card, -1);
		id = (id + 1) % 3;
		m_pServer->SendParams(m_client[id].socket, "SHOW_DISCARD", card, 1);
	}
}

bool Landlord::IsPermittedDiscard(const Holds& hold) const
{
	assert(false);
	return false;
}

void Landlord::Landlord_HandleRawData(SOCKET s, const std::string& data)
{
	printf("Landlord_HandleRawData data length: %d from socket %lld\n", (int)data.size(), s);
	std::string first = SockProto::GetHeadOne(data);
	std::string funcname;
	SockProto::Decode(first, funcname);
	std::string args = data;
	SockProto::RemoveHeadOne(args);
	auto it = m_mapFunc.find(funcname);
	if (m_mapFunc.find(funcname) != m_mapFunc.end())
	{
		printf("Landlord call function: %s\n", funcname.c_str());
		auto& func = m_mapFunc[funcname];
		func(args, s);
	}
	else
	{
		printf("Landlord has no function named: %s\n", funcname.c_str());
	}
}

bool Landlord::NeedToBind() const
{
	return m_client[0].socket == SOCKET_ERROR || m_client[1].socket == SOCKET_ERROR || m_client[2].socket == SOCKET_ERROR;
}

void Landlord::READY(std::string args, SOCKET s)
{
	int id = FindPlayer(s);
	int state;
	SockProto::Decode(args, state);
	if (id != -1)
	{
		m_client[id].ready = state;
	}
}

void Landlord::CALL_LORD(std::string args, SOCKET s)
{
	int id = this->FindPlayer(s);
	if (id == -1) return;
	if (id != m_expClient) return;
	int bCall = 0;
	SockProto::Decode(args, bCall);
	if (bCall == 1)
	{
		m_expClient = id;
		for (auto c : m_deck.cards)
		{
			m_client[id].hold.Insert(c);
		}
		m_client[id].hold.Sort();
		m_client[id].bLord = true;
		std::string c = m_client[id].hold.Encode();
		m_pServer->SendParams(s, std::string("DISPATCH_CARDS"), c);
	}
	else
	{
		id = (id + 1) % 3;
		NotifyYouCall(m_client[id].socket);
	}
}

void Landlord::DISCARD(std::string args, SOCKET s)
{
	if (m_client[m_expClient].socket != s)
		return;
	std::string cardStr;
	SockProto::Decode(args, cardStr);
	if (cardStr == "")
	{
		m_expClient = (m_expClient + 1) % 3;
		this->NotifyYouTurn(m_client[m_expClient].socket);
		return;
	}
	Holds h;
	h.Decode(cardStr);
	if (m_client[m_expClient].hold.Contains(h))
	{
		if (IsPermittedDiscard(h))
		{
			m_histroy.push_back({ h, m_expClient });
			this->ShowDiscard(s, h.Encode());
			m_client[m_expClient].hold.Remove(h);
			std::string outCard = h.Encode();
			m_pServer->SendParams(m_client[m_expClient].socket, "RECOGNIFY_DISCARD", outCard);
			m_expClient = (m_expClient + 1) % 3;
			this->NotifyYouTurn(m_client[m_expClient].socket);
			return;
		}
	}
	printf("Landlord::DISCARD rejected %lld.\n", s);
	m_pServer->SendParams(s, "DISCARD_REJECT");

}
