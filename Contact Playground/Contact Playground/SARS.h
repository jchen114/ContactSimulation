#pragma once

#include "State.h"
#include "Action.h"
#include "Reward.h"

class SARS
{
public:
	SARS(State s, Action a, Reward r, State s_n);
	SARS();
	~SARS();

	void SerializeCurrStateToString(std::string &str);
	void SerializeNextStateToString(std::string &str);
	int GetAction();
	float GetRewardValue();
	State GetCurrentState();
	State GetNextState();

private:

	void SerializeStateToString(State s, std::string &str);

	State m_s;
	Action m_a;
	Reward m_r;
	State m_s_n;
};

