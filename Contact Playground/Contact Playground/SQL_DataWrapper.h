#pragma once

#include <LinearMath\btVector3.h>
#include <string>
#include <vector>

class SQL_DataWrapper
{
public:
	SQL_DataWrapper(
		btVector3 TORSO_LV,
		float TORSO_O,
		float TORSO_AV,
		float URL_O,
		float URL_AV,
		float ULL_O,
		float ULL_AV,
		float LRL_O, 
		float LRL_AV, 
		float LLL_O, 
		float LLL_AV, 
		float RF_O,
		float RF_AV,
		float LF_O,
		float LF_AV,
		btVector3 RF_FORCES,
		btVector3 LF_FORCES,
		float GROUND_STIFFNESS,
		float GROUND_SLOPE,
		int SEQUENCE_ID,
		int SEQ_ORDER
		);
	~SQL_DataWrapper();

	btVector3 m_TORSO_LV;
	float m_TORSO_O;
	float m_TORSO_AV;
	float m_URL_O;
	float m_URL_AV;
	float m_ULL_O;
	float m_ULL_AV;
	float m_LRL_O;
	float m_LRL_AV;
	float m_LLL_O;
	float m_LLL_AV;
	float m_RF_O;
	float m_RF_AV;
	float m_LF_O;
	float m_LF_AV;
	btVector3 m_RF_FORCES;
	btVector3 m_LF_FORCES;
	float m_GROUND_STIFFNESS;
	float m_GROUND_SLOPE;
	int m_SEQUENCE_ID;
	int m_SEQ_ORDER;

};

void BuildStringFromForces(std::string &str, std::vector<btVector3> forces);