#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
struct PokerCard
{
	enum 
	{
		DIAMOND = 0,
		CLUB,
		HEART,
		SPADE,
		SMALLKING,
		BIGKING
	};
	int16_t color;
	int16_t number = 0;
	bool operator==(const PokerCard& other)const {
		return color == other.color && number == other.number;
	}
	bool operator!=(const PokerCard& other)const
	{
		return !(*this == other);
	}
	bool operator<(const PokerCard& other)const {
		return this->ToInt() < other.ToInt();
	}
	int ToInt()const {
		return number * 256 * 256 + color;
	}
	void FromInt(int num) {
		number = num >> 16;
		color = num - number * 256 * 256;
	}
};

//A deck is consists of 54 cards
class Deck
{
public:
	std::vector<PokerCard> cards;
public:
	Deck();
	void Shuffle();
	PokerCard PopOne() {
		PokerCard c = cards.back();
		cards.pop_back();
		return c;
	}
};

//Holds is a collection of cards
class Holds
{
protected:
	int GetNumberCount(int16_t number)const;
	bool IsSTRAIGHT()const;
	bool IsQUADDUAL()const;
	bool IsTWOTRIPLE()const;
	bool IsDUALTHREE()const;
	bool IsFLIGHTONE()const;
	bool IsFLIGHTTWO()const;
public:
	std::vector<PokerCard> cards;
public:
	enum
	{
		NONE = 0,
		SINGLE,			//����
		DUAL,			//����	
		TRIPLE,			//����
		BOMB,			//ը��
		QUADDUAL,		//�Ĵ���
		TRIPLEONE,		//����һ
		TRIPLETWO,		//������
		TWOTRIPLE,		//��������
		DUALTHREE,		//������
		STRAIGHT,		//˳��
		FLIGHTONE,		//��һ���ķɻ�
		FLIGHTTWO,		//�������ķɻ�
		ROCKET,			//��ը
	};
	Holds() {}
	void Insert(PokerCard c) {
		cards.push_back(c);
	}
	void Remove(PokerCard c) {
		auto it = std::find(cards.begin(), cards.end(), c);
		if (it != cards.end())
		{
			cards.erase(it);
		}
	}
	void RemoveNumber(int16_t number)
	{
		auto pred = [=](const PokerCard& c) {
			return number == c.number;
		};
		std::vector<PokerCard>::iterator it;
		while ((it = std::find_if(cards.begin(), cards.end(), pred)) != cards.end())
		{
			cards.erase(it);
		}
	}
	void Clear() {
		cards.clear();
	}
	std::string Encode()const {
		std::string ans = "";
		for (size_t i = 0; i < cards.size(); ++i)
		{
			ans += std::to_string(cards[i].ToInt());
			if (i + 1 != cards.size())
			{
				ans += ",";
			}
		}
		return ans;
	}
	void Sort() {
		std::sort(cards.begin(), cards.end());
	}
	void Decode(const std::string& cardStr);
	bool Contains(const Holds& other)const;
	void Remove(const Holds& other);
	int Type()const;
	bool operator>(const Holds& other)const;
};



std::vector<std::string> string_split(const std::string& s, char delimiter);