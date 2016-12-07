#include "stdafx.h"
#include "Action.h"

// For the mean distance and variance separation between bumps
#include "ContactLearningApp.h"

Action::Action(AgentAction action)
{
	m_myAction = action;
}

Action::Action()
{

}

Action::~Action()
{
}

float Action::GetDistanceToMove() {
	float dist = 0.0f;
	switch (m_myAction)
	{
	case MOVE_RIGHT_1: {
		dist = m_mean_separation - 1.5 * m_variance;
	}
		break;
	case MOVE_RIGHT_2: {
		dist = m_mean_separation - 1.0 * m_variance;
	}
		break;
	case MOVE_RIGHT_3: {
		dist = m_mean_separation - 0.5 * m_variance;
	}
		break;
	case MOVE_RIGHT_4: {
		dist = m_mean_separation;
	}
		break;
	case MOVE_RIGHT_5: {
		dist = m_mean_separation + 0.5 * m_variance;
	}
		break;
	case MOVE_RIGHT_6: {
		dist = m_mean_separation + 1.0 * m_variance;
	}
		break;
	case MOVE_RIGHT_7: {
		dist = m_mean_separation + 1.5 * m_variance;
	}
		break;
	default:
		dist = m_mean_separation;
		break;
	}
	printf("dist = %f\n", dist);
	return dist;
}

int Action::GetAction() {
	return m_myAction;

}