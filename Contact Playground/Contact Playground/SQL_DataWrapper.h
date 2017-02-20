#pragma once

#include <LinearMath\btVector3.h>
#include <string>
#include <vector>
#include <tuple>

class SQL_DataWrapper
{
public:
	SQL_DataWrapper(
		const btVector3 &TORSO_LV,
		std::vector<std::tuple<float, float, float>> DsOsAVs,
		std::vector<btVector3> RF_FORCES,
		std::vector<btVector3> LF_FORCES,
		float GROUND_STIFFNESS,
		float GROUND_DAMPING,
		float GROUND_SLOPE,
		int SEQUENCE_ID,
		int SEQ_ORDER
		);

	~SQL_DataWrapper();

	btVector3 m_TORSO_LV;
	std::vector<std::tuple<float, float, float>> m_DsOsAVS;
	std::vector<btVector3> m_RF_FORCES;
	std::vector<btVector3> m_LF_FORCES;
	float m_GROUND_STIFFNESS;
	float m_GROUND_DAMPING;
	float m_GROUND_SLOPE;
	int m_SEQUENCE_ID;
	int m_SEQ_ORDER;

};

void BuildStringFromForces(std::string &str, std::vector<btVector3> forces);