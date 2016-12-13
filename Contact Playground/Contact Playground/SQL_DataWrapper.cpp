#include "stdafx.h"
#include "SQL_DataWrapper.h"


SQL_DataWrapper::SQL_DataWrapper(
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
	)
{

	btVector3 m_TORSO_LV = TORSO_LV;
	float m_TORSO_O = TORSO_O;
	float m_TORSO_AV = TORSO_AV;
	float m_URL_O = URL_O;
	float m_URL_AV = URL_AV;
	float m_ULL_O = ULL_O;
	float m_ULL_AV = ULL_AV;
	float m_LRL_O = LRL_O;
	float m_LRL_AV = LRL_AV;
	float m_LLL_O = LLL_O;
	float m_LLL_AV = LLL_AV;
	float m_RF_O = RF_O;
	float m_RF_AV = RF_AV;
	float m_LF_O = LF_O;
	float m_LF_AV = LF_AV;
	btVector3 m_RF_FORCES = RF_FORCES;
	btVector3 m_LF_FORCES = LF_FORCES;
	float m_GROUND_STIFFNESS = GROUND_STIFFNESS;
	float m_GROUND_SLOPE = GROUND_SLOPE;
	int m_SEQUENCE_ID = SEQUENCE_ID;
	int m_SEQ_ORDER = SEQ_ORDER;

}


SQL_DataWrapper::~SQL_DataWrapper()
{
}

void BuildStringFromForces(std::string &str, std::vector<btVector3> forces) {

}