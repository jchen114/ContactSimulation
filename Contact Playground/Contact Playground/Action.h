#pragma once

enum AgentAction{MOVE_RIGHT_1 = 0, MOVE_RIGHT_2, MOVE_RIGHT_3, MOVE_RIGHT_4, MOVE_RIGHT_5, MOVE_RIGHT_6, MOVE_RIGHT_7};

class Action
{
public:
	Action(AgentAction action);
	Action();
	~Action();

	float GetDistanceToMove();
	int GetAction();

private:

	AgentAction m_myAction;

};

