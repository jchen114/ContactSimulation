#include "stdafx.h"
#include "SQL_DataWrapper.h"
#include <sstream>


SQL_DataWrapper::SQL_DataWrapper(
	const btVector3 &TORSO_LV,
	std::vector<float> OsAndAVs,
	std::vector<btVector3> RF_FORCES,
	std::vector<btVector3> LF_FORCES,
	float GROUND_STIFFNESS,
	float GROUND_DAMPING,
	float GROUND_SLOPE,
	int SEQUENCE_ID,
	int SEQ_ORDER
	)
{

	m_TORSO_LV = TORSO_LV;
	m_OsAndAVS = OsAndAVs;
	std::vector<btVector3> m_RF_FORCES = RF_FORCES;
	std::vector<btVector3> m_LF_FORCES = LF_FORCES;
	m_GROUND_STIFFNESS = GROUND_STIFFNESS;
	m_GROUND_DAMPING = GROUND_DAMPING;
	m_GROUND_SLOPE = GROUND_SLOPE;
	m_SEQUENCE_ID = SEQUENCE_ID;
	m_SEQ_ORDER = SEQ_ORDER;

	//printf("SQL DATA Wrapper Stiffness = %f, Damping = %f, Slope = %f\n", GROUND_STIFFNESS, GROUND_DAMPING, GROUND_SLOPE);

}


SQL_DataWrapper::~SQL_DataWrapper()
{
}

void BuildStringFromForces(std::string &str, std::vector<btVector3> forces) {

	std::stringstream ss;

	for (auto force : forces) {
		ss << force.x() << ", " << force.y() << " | ";
	}

	str = ss.str();
	str = str.substr(0, str.size() - 2);

	//printf("str = %s\n", str.c_str());

}