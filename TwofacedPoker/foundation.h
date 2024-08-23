// foundation.h
#pragma once
#include <string>

// ���� �̸��� �¸� Ƚ�� ǥ��
class User
{
private:
	std::string name;
	int winCount;
public:
	User(std::string name, int winCount) : name(name), winCount(winCount) { }

	std::string getName() const {
		return name;
	}
	int getwinCount() const {
		return winCount;
	}
};

// ī���� �ո� �޸�
class Card
{
private:
	int front;
	int back;
public:
	Card(int front, int back) : front(front), back(back) { }

	int getFront() const {
		return front;
	}
	int getBack() const {
		return back;
	}
};