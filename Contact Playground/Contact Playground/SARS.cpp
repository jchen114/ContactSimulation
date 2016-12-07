#include "stdafx.h"
#include "SARS.h"
#include <sstream>


SARS::SARS()
{
}


SARS::SARS(State s, Action a, Reward r, State s_n) {

	m_s = s;
	m_a = a;
	m_r = r;
	m_s_n = s_n;

}

SARS::~SARS()
{
}

int SARS::GetAction() {

	return (int)m_a.GetAction();
}

float SARS::GetRewardValue() {
	return m_r.GetReward();
}

void SARS::SerializeCurrStateToString(std::string &str) {
	SerializeStateToString(m_s, str);
}

void SARS::SerializeNextStateToString(std::string &str) {
	SerializeStateToString(m_s_n, str);
}

void SARS::SerializeStateToString(State s, std::string &str) {

	std::vector<std::tuple<float, float>> state = s.GetContactState();

	std::stringstream ss;
	for (int i = 0; i < state.size(); i++) {
		std::tuple<float, float> tup = state[i];
		ss << "(" << std::get<0>(tup) << ", " << std::get<1>(tup) << ")";
		if (i != state.size() - 1) {
			ss << " | ";
		}
	}
	str = ss.str();
}

State SARS::GetCurrentState() {
	return m_s;
}

State SARS::GetNextState() {
	return m_s_n;
}