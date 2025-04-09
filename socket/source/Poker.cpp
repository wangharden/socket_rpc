#include "Poker.h"
#include <algorithm>
#include <random>
#include <string>
#include <sstream>
#include <map>
Deck::Deck()
{
	cards.reserve(54);
	for (int i = 1; i <= 13; ++i)
	{
		cards.push_back({ PokerCard::SPADE, (int16_t) i });
		cards.push_back({ PokerCard::HEART, (int16_t)i });
		cards.push_back({ PokerCard::CLUB, (int16_t)i });
		cards.push_back({ PokerCard::DIAMOND, (int16_t)i });
	}
	cards.push_back({ PokerCard::SMALLKING, 0 });
	cards.push_back({ PokerCard::BIGKING, 0 });
}

void Deck::Shuffle()
{
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(cards.begin(), cards.end(), g);
}

std::vector<std::string> string_split(const std::string& s, char delimiter)
{
	using namespace std;
	vector<string> tokens;
	istringstream iss(s);
	string token;
	while (std::getline(iss, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

int Holds::GetNumberCount(int16_t number) const
{
	int cnt = 0;
	for (auto& c : cards)
	{
		if (c.number == number)
			++cnt;
	}
	return cnt;
}

bool Holds::IsSTRAIGHT() const
{
	if (cards.size() < 5) return false;
	auto tmp = (*this);
	tmp.Sort();
	if (tmp.cards[0].number == 0) return false;
	if (tmp.cards[0].number == 1)
	{
		PokerCard c = tmp.cards[0];
		tmp.cards.erase(tmp.cards.begin());
		tmp.cards.push_back(c);
	}
	for (int i = 0; i < tmp.cards.size() - 1; ++i)
	{
		if (tmp.cards[i + 1].number - tmp.cards[i].number != 1)
			return false;
	}
	return true;
}

bool Holds::IsQUADDUAL() const
{
	if (cards.size() != 6) return false;
	auto tmp = (*this);
	tmp.Sort();
	auto& vec = tmp.cards;
	if (vec[0].number == vec[1].number && vec[0].number != vec[2].number)
	{
		vec.erase(vec.begin());
		vec.erase(vec.begin());
		if (tmp.Type() == BOMB)
			return true;
		return false;
	}
	if (vec[4].number == vec[5].number)
	{
		vec.pop_back();
		vec.pop_back();
		if (tmp.Type() == BOMB)
			return true;
		return false;
	}
	return false;
}

bool Holds::IsTWOTRIPLE() const
{
	std::map<int16_t, int> m;
	for (auto c : cards)
	{
		if (m.find(c.number) == m.end())
		{
			m[c.number] = 1;
		}
		else
		{
			m[c.number] += 1;
		}
	}
	if (m.size() == 2)
	{
		for (auto& v : m)
		{
			if (v.second != 3)
				return false;
		}
		auto it = m.begin();
		auto it1 = it;
		++it1;
		if (std::abs((*it).first - (*it1).first) == 1)
			return true;
		return false;
	}
	return false;
}

bool Holds::IsDUALTHREE() const
{
	if (cards.size() != 6) return false;
	auto tmp = (*this);
	tmp.Sort();
	int16_t first = tmp.cards[0].number;
	int16_t center = tmp.cards[2].number;
	int16_t last = tmp.cards[4].number;
	int nf = tmp.GetNumberCount(first);
	int nc = tmp.GetNumberCount(center);
	int nl = tmp.GetNumberCount(last);
	if (nf == 2 && nc == 2 && nl == 2)
	{
		if (first == 0)
			return false;
		if (first == 1)
		{
			return center == 12 && last == 13;
		}
		else
		{
			return first + 1 == center && center + 1 == last;
		}
	}
	else
		return false;
}

bool Holds::IsFLIGHTONE() const
{
	if (cards.size() != 8)
		return false;
	auto tmp = (*this);
	tmp.Sort();
	int n0 = tmp.GetNumberCount(tmp.cards[2].number);
	int n1 = tmp.GetNumberCount(tmp.cards[5].number);
	if (n0 == 3 && n1 == 3)
	{
		if (tmp.cards[2].number == 1)
		{
			return tmp.cards[5].number == 13;
		}
		else
			return tmp.cards[2].number + 1 == tmp.cards[5].number;
	}
	else
	{
		return false;
	}
}

bool Holds::IsFLIGHTTWO() const
{
	if (cards.size() != 10)
		return false;
	auto tmp = (*this);
	tmp.Sort();
	int cnt = 0;
	int i = 0;
	do
	{
		cnt = tmp.GetNumberCount(tmp.cards[i].number);
	} while (cnt != 3 && ++i < 10);
	if (i == 10)
		return false;
	int n0 = tmp.cards[i].number;
	int n1 = n0 == 1 ? 13 : n0 + 1;
	tmp.RemoveNumber(n0);
	cnt = tmp.GetNumberCount(n1);
	if (cnt != 3)
		return false;
	tmp.RemoveNumber(n1);
	cnt = tmp.GetNumberCount(tmp.cards[0].number);
	if (cnt != 2)
		return false;
	tmp.RemoveNumber(tmp.cards[0].number);
	cnt = tmp.GetNumberCount(tmp.cards[0].number);
	if (cnt != 2)
		return false;
	return true;
}

void Holds::Decode(const std::string& cardStr)
{
	std::vector<std::string> cardvec = string_split(cardStr, ',');
	for (auto s : cardvec)
	{
		PokerCard c;
		c.FromInt(std::atoi(s.c_str()));
		cards.push_back(c);
	}
}

bool Holds::Contains(const Holds& other)const
{
	if (cards.size() < other.cards.size())
		return false;
	for (auto c : other.cards)
	{
		if (std::find(cards.begin(), cards.end(), c) == cards.end())
			return false;
	}
	return true;
}

void Holds::Remove(const Holds& other)
{
	for (auto& c : other.cards)
	{
		this->Remove(c);
	}
}

int Holds::Type() const
{
	if (cards.size() == 0)
		return NONE;
	if (cards.size() == 1)
		return SINGLE;
	if (cards.size() == 2) {
		if (cards[0].number == 0 && cards[1].number == 0)
			return ROCKET;
		if (cards[0].number == cards[1].number)
			return DUAL;
		return NONE;
	}
	if (cards.size() == 3)
	{
		if (cards[0].number == cards[1].number && cards[0].number == cards[2].number)
			return TRIPLE;
		return NONE;
	}
	if (cards.size() == 4)
	{
		if (cards[0].number == cards[1].number && cards[0].number == cards[2].number &&
			cards[0].number == cards[3].number)
			return BOMB;
		if (cards[0].number == cards[1].number && cards[0].number == cards[2].number)
			return TRIPLEONE;
		if (cards[0].number == cards[1].number && cards[0].number == cards[3].number)
			return TRIPLEONE;
		if (cards[2].number == cards[1].number && cards[2].number == cards[3].number)
			return TRIPLEONE;
		return NONE;
	}
	if (cards.size() == 5) {
		if (cards[0].number == cards[1].number &&
			cards[2].number == cards[3].number && cards[2].number == cards[4].number)
			return TRIPLETWO;
		if (cards[0].number == cards[1].number && cards[0].number == cards[2].number &&
			cards[3].number == cards[4].number)
			return TRIPLETWO;
		if (IsSTRAIGHT())
			return STRAIGHT;
		return NONE;
	}
	if (cards.size() == 6) {
		if (IsTWOTRIPLE())
			return TWOTRIPLE;
		if (IsQUADDUAL())
			return QUADDUAL;
		if (IsDUALTHREE())
			return DUALTHREE;
		return NONE;
	}
	if (cards.size() == 7)
	{
		if (IsSTRAIGHT())
			return STRAIGHT;
	}
	if (cards.size() == 8) {
		if (IsFLIGHTONE())
			return FLIGHTONE;
		if (IsSTRAIGHT())
			return STRAIGHT;
	}
	if (cards.size() == 9)
	{
		if (IsSTRAIGHT())
			return STRAIGHT;
		//may be three triple
	}
	if (cards.size() == 10)
	{
		if (IsSTRAIGHT())
			return STRAIGHT;
		if (IsFLIGHTTWO())
			return FLIGHTTWO;
	}
	return NONE;
}

bool Holds::operator>(const Holds& other) const
{
	int t = this->Type();
	int ot = other.Type();
	if (t == NONE)
		return false;
	if (ot == NONE)
		return false;
	if (t == ot)
	{
		switch (t)
		{
		case SINGLE:
		case DUAL:
		case TRIPLE:
		case BOMB:
			return cards[0].number > other.cards[0].number;
		default:
			break;
		}
	}
	else
	{
		switch (t)
		{
		case BOMB:
		case ROCKET:
			return true;
		default:
			break;
		}
	}
	return false;
}
