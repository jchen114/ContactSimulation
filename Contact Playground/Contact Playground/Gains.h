#pragma once

enum AssociatedBody{TORSO, UPPER_RIGHT_LEG, UPPER_LEFT_LEG, LOWER_RIGHT_LEG, LOWER_LEFT_LEG, RIGHT_FOOT, LEFT_FOOT};

class Gains
{
public:
	Gains();
	Gains(float kp, float kd, AssociatedBody body) {
		m_kp = kp;
		m_kd = kd;
		m_associatedBody = body;
	}
	~Gains();

	float m_kp;
	float m_kd;

	AssociatedBody GetAssociatedBody() {
		return m_associatedBody;
	}

private:
	AssociatedBody m_associatedBody;

};

