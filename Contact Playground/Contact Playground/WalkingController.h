#pragma once
#include <vector>
#include <cstdio>
#include <ctime>
#include <unordered_map>

#include "LinearMath\btQuickprof.h"
#include "LinearMath\btVector3.h"

class RagDoll;
class State;
class Gains;


#pragma region DEFINITIONS 

#define TORQUE_LIMIT 300

// MASS
#define torso_mass 70
#define upper_leg_mass 5
#define lower_leg_mass 4
#define feet_mass 1

// DIMENSIONS. 
#define torso_height SCALING_FACTOR * 0.48f
#define torso_width torso_height / 5

#define upper_leg_height SCALING_FACTOR * 0.45f
#define upper_leg_width upper_leg_height / 5

#define lower_leg_height SCALING_FACTOR * 0.45f
#define lower_leg_width lower_leg_height / 7

#define foot_height SCALING_FACTOR * 0.05f
#define foot_width foot_height * 4

#define MARKER_COLOR btVector3(255,255,0)
#define DRAW_SPEEDUP 10

// Gains
#define KP_LOWER 0.0f
#define KP_HIGHER 9000.0f
#define KD_LOWER 0.0f
#define KD_HIGHER 900.0f

// Spinner limits
#define SPINNER_TORSO_LOW		-90.0f
#define SPINNER_TORSO_HIGH		90.0f
#define SPINNER_UPPER_LEG_LOW	-90.0f
#define SPINNER_UPPER_LEG_HIGH	90.0f
#define SPINNER_LOWER_LEG_LOW	0.0f
#define SPINNER_LOWER_LEG_HIGH	150.0f
#define SPINNER_FOOT_LOW		-15.0f
#define SPINNER_FOOT_HIGH		90.0f

// Hinge limits
#define HINGE_TORSO_ULL_LOW		-90.0f
#define HINGE_TORSO_ULL_HIGH	90.0f

#define HINGE_TORSO_URL_LOW		-90.0f
#define HINGE_TORSO_URL_HIGH	90.0f

#define HINGE_ULL_LLL_LOW		0.0f
#define HINGE_ULL_LLL_HIGH		150.0f

#define HINGE_URL_LRL_LOW		0.0f
#define HINGE_URL_LRL_HIGH		150.0f

#define HINGE_LLL_LF_LOW		-15.0f
#define HINGE_LLL_LF_HIGH		90.0f

#define HINGE_LRL_RF_LOW		-15.0f
#define HINGE_LRL_RF_HIGH		90.0f


#pragma endregion DEFINITIONS


enum CurrentRagDollState { STATE_0 = 100, STATE_1, STATE_2, STATE_3, STATE_4, STATE_5 };
enum CurrentControllerState {WALKING = 105, PAUSE, RESET};

class WalkingController
{
	
public:
	WalkingController();
	WalkingController(RagDoll *ragDoll);
	
	~WalkingController();

	std::vector<std::string> GetGaits();
	std::vector<State *> ReadStates(std::string state_dir);
	std::vector<State *> ReadStateFile();
	std::vector<Gains *> ReadGains(std::string gains_dir);
	std::vector<Gains *> ReadGainsFile();
	std::vector<float> ReadFeedback(std::string fdbk_dir);
	std::vector<float> ReadFeedbackFile();
	float ReadTime(std::string time_dir);
	float ReadTimeFile();

	void SaveStates();
	void SaveStates(std::string gait);
	void SaveGains();
	void SaveGains(std::string gait);
	void SaveFeedback();
	void SaveFeedback(std::string gait);
	void SaveTime();
	void SaveTime(std::string gait);

	void StateLoop();
	void InitiateWalking();
	void PauseWalking();
	void Reset();
	void NotifyLeftFootGroundContact();
	void NotifyRightFootGroundContact();
	void NotifyTorsoGroundContact();
	void ChangeGait(std::string gait);

	void SetState1(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot);
	void SetState2(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot);
	void SetState3(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot);
	void SetState4(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot);

	void SetTorsoGains(float kp, float kd);
	void SetUpperLeftLegGains(float kp, float kd);
	void SetUpperRightLegGains(float kp, float kd);
	void SetLowerLeftLegGains(float kp, float kd);
	void SetLowerRightLegGains(float kp, float kd);
	void SetLeftFootGains(float kp, float kd);
	void SetRightFootGains(float kp, float kd);

	void SetFeedback1(float cd, float cv);
	void SetFeedback2(float cd, float cv);

	void SetStateTime(float time);

	CurrentControllerState m_currentState = RESET;
	CurrentRagDollState m_ragDollState = STATE_0;

	btVector3 m_stanceAnklePosition = btVector3(0, 0, 0);
	btVector3 m_COMPosition = btVector3(0,0,0);

	std::unordered_map<std::string, std::vector<State*>> m_GaitMap;
	std::unordered_map<std::string, std::vector<Gains *>> m_GainMap;
	std::unordered_map<std::string, std::vector<float>> m_FdbkMap;
	std::unordered_map<std::string, float> m_TmMap;

	// Set these in the GUI
	Gains *m_torso_gains;
	Gains *m_ull_gains;
	Gains *m_url_gains;
	Gains *m_lll_gains;
	Gains *m_lrl_gains;
	Gains *m_lf_gains;
	Gains *m_rf_gains;

	State *m_state0;
	State *m_state1;
	State *m_state2;
	State *m_state3;
	State *m_state4;

	float *m_cd_1;
	float *m_cv_1;
	float *m_cd_2;
	float *m_cv_2;

	float m_state_time = 0.0f;

	// Temporary
	Gains *m_torso_gains_tmp;
	Gains *m_ull_gains_tmp;
	Gains *m_url_gains_tmp;
	Gains *m_lll_gains_tmp;
	Gains *m_lrl_gains_tmp;
	Gains *m_lf_gains_tmp;
	Gains *m_rf_gains_tmp;

	State *m_state0_tmp;
	State *m_state1_tmp;
	State *m_state2_tmp;
	State *m_state3_tmp;
	State *m_state4_tmp;

	float *m_cd_1_tmp;
	float *m_cv_1_tmp;
	float *m_cd_2_tmp;
	float *m_cv_2_tmp;

	float m_state_time_tmp = 0.0f;

	bool m_changeGait = false;
	bool m_wait = true;

	std::string m_currentGait;

private:

	bool m_leftFootGroundHasContacted = false;
	bool m_rightFootGroundHasContacted = false;
	bool m_torsoHasContacted = false;

	RagDoll *m_ragDoll;

	btClock m_clock;
	double m_duration = 0.0f;
	bool m_reset = true;

	std::vector<float> CalculateState1Torques();
	std::vector<float> CalculateState2Torques();
	std::vector<float> CalculateState3Torques();
	std::vector<float> CalculateState4Torques();

	float CalculateFeedbackSwingHip();

	float CalculateTorqueForTorso(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForUpperLeftLeg(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForUpperRightLeg(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForLowerLeftLeg(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForLowerRightLeg(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForLeftFoot(float targetPosition, float currentPosition, float currentVelocity);
	float CalculateTorqueForRightFoot(float targetPosition, float currentPosition, float currentVelocity);

	float CalculateTorque(float kp, float kd, float targetPosition, float currentPosition, float velocity);

};
