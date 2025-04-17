#include "Game.h"

std::string Game::GetGameName(int type)
{
	switch (type)
	{
	case GOBANG_GAME:
		return "Gobang";
	default:
		return "BaseGame";
		break;
	}
}

int Game::GetGameCapcity(int type)
{
	switch (type)
	{
	case GOBANG_GAME:
		return 2;
	default:
		return 0;
		break;
	}
}

void Game::Init_Base()
{
	if (this->NeedToBind())
	{
		bool b0 = m_pServer->Bind(IP, Port);
		if (!b0)
		{
			int err = WGetLastError();
			printf("bind failed, error code %d\n", err);
			m_bCloseGame = true;
			return;
		}
		b0 = m_pServer->Listen(5);
	}
	m_bReadyTick = true;
}

void Game::Tick_Base()
{
	if (m_pServer) {
		m_pServer->Tick();
	}
}

void Game::DeInit_Base()
{
	if (m_pServer)
	{
		delete m_pServer;
		m_pServer = nullptr;
	}
}

Game::Game()
{
	m_pServer = new BlockedServer();
}
