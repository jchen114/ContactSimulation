#pragma once

class State {

public:

	State(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot) {
		m_torsoAngle = torso;
		m_upperLeftLegAngle = upperLeftLeg;
		m_upperRightLegAngle = upperRightLeg;
		m_lowerLeftLegAngle = lowerLeftLeg;
		m_lowerRightLegAngle = lowerRightLeg;
		m_leftFootAngle = leftFoot;
		m_rightFootAngle = rightFoot;

	}

	State();
	~State();

	float m_torsoAngle;
	float m_upperRightLegAngle;
	float m_upperLeftLegAngle;
	float m_lowerRightLegAngle;
	float m_lowerLeftLegAngle;
	float m_rightFootAngle;
	float m_leftFootAngle;


};
