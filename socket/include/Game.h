#pragma once
#include "Utility.h"

class Game
{
public:
	enum {
		BASE_GAME = 0,
		GOBANG_GAME,
	};
	static std::string GetGameName(int type);
	static int GetGameCapcity(int type);
private:
	void Init_Base();
	void Tick_Base();
	void DeInit_Base();
protected:
	bool m_bCloseGame = false;
	bool m_bReadyTick = false;
	BlockedServer* m_pServer = nullptr;
	virtual void Init() {}
	virtual void Tick() {}
	virtual void DeInit() {}
public:
	Game();
 	void Start() {
		Init_Base();
		Init();
		printf("Game Init finished\n");
		if (m_bReadyTick)
		{
			while (!m_bCloseGame)
			{
				Tick_Base();
				Tick();
			}
		}
		else
		{
			printf("Game not ready to tick\n");
		}
		DeInit();
		DeInit_Base();
		printf("Game DeInit finished\n");
	}
	virtual ~Game() {}
public:
	std::string IP = "127.0.0.1";
	int Port = 1009;
	virtual bool NeedToBind()const { return true; }
};