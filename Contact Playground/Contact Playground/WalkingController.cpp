#include "stdafx.h"

#include "WalkingController.h"
#include "RagDoll.h"
#include "State.h"
#include "Gains.h"

#include "dirent.h"
#include <fstream>

#include <limits.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include <iostream>
#include <fstream>

WalkingController::WalkingController()
{
}

WalkingController::WalkingController(RagDoll *ragDoll) {
	m_ragDoll = ragDoll;
	m_ragDollState = STATE_0;
	m_currentState = RESET;	
	m_COMPosition = btVector3(0, 0, 0);
	m_stanceAnklePosition = btVector3(0, 0, 0);

}

WalkingController::~WalkingController()
{
}

#pragma region FILE_IO

std::vector<std::string> WalkingController::GetGaits() {
	DIR *dir;
	struct dirent *ent;
	std::vector<std::string> gaits;
	std::string gaits_dir = "..\\..\\State Configurations\\Gaits";
	if ((dir = opendir(gaits_dir.c_str())) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_DIR) {
				std::string dirName = ent->d_name;
				if (strcmp(dirName.c_str(), ".") == 0 || strcmp(dirName.c_str(), "..") == 0) {
					continue;
				}
				printf("Directory name: %s\n", dirName.c_str());
				gaits.push_back(dirName);
				std::string gait_dir = gaits_dir + "\\" + dirName;
				// Gait state
				std::vector <State *> states = ReadStates(gait_dir);
				m_GaitMap.insert(std::pair<std::string, std::vector<State *>>(dirName, states));
				// Gains
				std::vector<Gains*> gains = ReadGains(gait_dir);
				m_GainMap.insert(std::pair<std::string, std::vector<Gains *>>(dirName, gains));
				// Feedback
				std::vector<float> fdbk = ReadFeedback(gait_dir);
				m_FdbkMap.insert(std::pair<std::string, std::vector<float>>(dirName, fdbk));
				// Time
				float time = ReadTime(gait_dir);
				m_TmMap.insert(std::pair<std::string, float>(dirName, time));
			}
		}
		closedir(dir);
	}
	return gaits;
}

std::vector<State *> WalkingController::ReadStates(std::string state_dir) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(state_dir.c_str())) != NULL) {
		std::string state_ext = "cfg";

		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG) {
				//printf("%s\n", ent->d_name);
				std::string fname = ent->d_name;
				if (fname.find(state_ext, (fname.length() - state_ext.length())) != std::string::npos) {
					std::stringstream ss;
					ss << state_dir << "\\" << ent->d_name;
					std::ifstream infile(ss.str());
					float torso, ull, url, lll, lrl, lf, rf;
					char c;
					//printf("States: \n");
					int state = 0;
					while ((infile >> torso >> c >> ull >> c >> url >> c >> lll >> c >> lrl >> c >> lf >> c >> rf) && (c == ',')) {
						//printf("%f, %f, %f, %f, %f, %f, %f \n", torso, ull, url, lll, lrl, lf, rf);
						// Set GLUI to read parameters
						switch (state)
						{
						case 0:
							m_state0 = new State(torso, ull, url, lll, lrl, lf, rf);
							break;
						case 1:
							m_state1 = new State(torso, ull, url, lll, lrl, lf, rf);
							break;
						case 2:
							m_state2 = new State(torso, ull, url, lll, lrl, lf, rf);
							break;
						case 3:
							m_state3 = new State(torso, ull, url, lll, lrl, lf, rf);
							break;
						case 4:
							m_state4 = new State(torso, ull, url, lll, lrl, lf, rf);
							break;
						default:
							break;
						}

						state++;
					}
				}
			}
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		// Initialize state to be zeros
		m_state0 = new State(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		m_state1 = new State(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		m_state2 = new State(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		m_state3 = new State(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		m_state4 = new State(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}

	return std::vector < State * > {m_state0, m_state1, m_state2, m_state3, m_state4};
}

std::vector<State *> WalkingController::ReadStateFile() {
	return ReadStates("..\\..\\State Configurations");
}

std::vector<Gains *>WalkingController::ReadGains(std::string gains_dir) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(gains_dir.c_str())) != NULL) {
		std::string gains_ext = "gns";
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG) {
				//printf("%s\n", ent->d_name);
				std::string fname = ent->d_name;
				if (fname.find(gains_ext, (fname.length() - gains_ext.length())) != std::string::npos) {
					std::stringstream ss;
					ss << gains_dir << "\\" << ent->d_name;
					std::ifstream infile(ss.str());
					float kp, kd;
					char c;
					int body = 0;
					while ((infile >> kp >> c >> kd) && (c == ',')) {
						//printf("%f, %f \n", kp, kd);
						// Set GLUI to read parameters
						switch (body)
						{
						case 0:
							// Torso
							m_torso_gains = new Gains(kp, kd, TORSO);
							break;
						case 1:
							// Upper Left Leg
							m_ull_gains = new Gains(kp, kd, UPPER_LEFT_LEG);
							break;
						case 2:
							// Upper right leg
							m_url_gains = new Gains(kp, kd, UPPER_RIGHT_LEG);
							break;
						case 3:
							// Lower Left leg
							m_lll_gains = new Gains(kp, kd, LOWER_LEFT_LEG);
							break;
						case 4:
							// Lower right leg
							m_lrl_gains = new Gains(kp, kd, LOWER_RIGHT_LEG);
							break;
						case 5:
							// Left foot
							m_lf_gains = new Gains(kp, kd, LEFT_FOOT);
							break;
						case 6:
							// Right foot
							m_rf_gains = new Gains(kp, kd, RIGHT_FOOT);
							break;
						default:
							break;
						}
						body++;
					}
				}
			}

		}
	}
	else {
		m_torso_gains = new Gains(0, 0, TORSO);
		m_ull_gains = new Gains(0, 0, UPPER_LEFT_LEG);
		m_url_gains = new Gains(0, 0, UPPER_RIGHT_LEG);
		m_lll_gains = new Gains(0, 0, LOWER_LEFT_LEG);
		m_lrl_gains = new Gains(0, 0, LOWER_RIGHT_LEG);
		m_lf_gains = new Gains(0, 0, LEFT_FOOT);
		m_rf_gains = new Gains(0, 0, RIGHT_FOOT);
	}

	std::vector<Gains *> gains = { m_torso_gains, m_ull_gains, m_url_gains, m_lll_gains, m_lrl_gains, m_lf_gains, m_rf_gains };
	return gains;
}

std::vector<Gains *> WalkingController::ReadGainsFile() {

	return ReadGains("..\\..\\State Configurations");
}

std::vector<float> WalkingController::ReadFeedback(std::string fdbk_dir) {
	DIR *dir;
	struct dirent *ent;
	float cd_1, cv_1, cd_2, cv_2;
	if ((dir = opendir(fdbk_dir.c_str())) != NULL) {
		std::string feedback_ext = "fdbk";
		
		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG) {
				//printf("%s\n", ent->d_name);
				std::string fname = ent->d_name;
				if (fname.find(feedback_ext, (fname.length() - feedback_ext.length())) != std::string::npos) {
					std::stringstream ss;
					ss << fdbk_dir << "\\" << ent->d_name;
					std::ifstream infile(ss.str());
					char c;
					float w, x, y, z;
					while ((infile >> w >> c >> x >> c >> y >> c >> z) && (c == ',')) {
						cd_1 = w;
						cv_1 = x;
						cd_2 = y;
						cv_2 = z;
						printf("fdbk = %f, %f, %f, %f \n", cd_1, cv_1, cd_2, cv_2);
					}
				}
			}
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		// Initialize to be zeros
		cd_1 = 0.0f;
		cv_1 = 0.0f;
		cd_2 = 0.0f;
		cv_2 = 0.0f;
	}

	std::vector<float> fdbk = { cd_1, cv_1, cd_2, cv_2 };
	return fdbk;
}

std::vector<float>WalkingController::ReadFeedbackFile() {
	return ReadFeedback("..\\..\\State Configurations");
}

float WalkingController::ReadTime(std::string time_dir) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(time_dir.c_str())) != NULL) {
		std::string feedback_ext = "tm";

		while ((ent = readdir(dir)) != NULL) {
			if (ent->d_type == DT_REG) {
				//printf("%s\n", ent->d_name);
				std::string fname = ent->d_name;
				if (fname.find(feedback_ext, (fname.length() - feedback_ext.length())) != std::string::npos) {
					std::stringstream ss;
					ss << time_dir << "\\" << ent->d_name;
					std::ifstream infile(ss.str());
					float time;
					while (infile >> time) {
						m_state_time = time;
					}
				}
			}
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		// Initialize to be zeros
		m_state_time = 0.0f;
	}

	return m_state_time;
}

float WalkingController::ReadTimeFile() {
	return ReadTime("..\\..\\State Configurations");
}

void WalkingController::SaveStates(std::string gait) {
	std::ofstream states_file;
	std::string file_name = "..\\..\\State Configurations\\Gaits\\" + gait + "\\states.cfg";
	printf("save states tp %s\n", file_name.c_str());
	states_file.open(file_name);
	std::vector<State *> states = { m_state0, m_state1, m_state2, m_state3, m_state4 };
	for (std::vector<State *>::iterator it = states.begin(); it != states.end(); it++) {
		char buffer[100];
		sprintf_s(buffer, "%f, %f, %f, %f, %f, %f, %f\n", (*it)->m_torsoAngle, (*it)->m_upperLeftLegAngle, (*it)->m_upperRightLegAngle, (*it)->m_lowerLeftLegAngle, (*it)->m_lowerRightLegAngle, (*it)->m_leftFootAngle, (*it)->m_rightFootAngle);
		std::cout << buffer;
		states_file << buffer;
	}
	states_file.close();
}

void WalkingController::SaveStates() {
	std::ofstream states_file;
	states_file.open("..\\..\\State Configurations\\states.cfg");
	std::vector<State *> states = { m_state0, m_state1, m_state2, m_state3, m_state4 };
	for (std::vector<State *>::iterator it = states.begin(); it != states.end(); it++) {
		char buffer[100];
		sprintf_s(buffer, "%f, %f, %f, %f, %f, %f, %f\n", (*it)->m_torsoAngle, (*it)->m_upperLeftLegAngle, (*it)->m_upperRightLegAngle, (*it)->m_lowerLeftLegAngle, (*it)->m_lowerRightLegAngle, (*it)->m_leftFootAngle, (*it)->m_rightFootAngle);
		std::cout << buffer;
		states_file << buffer;
	}
	states_file.close();
}

void WalkingController::SaveGains(std::string gait) {
	std::ofstream gains_file;
	gains_file.open("..\\..\\State Configurations\\Gaits\\" + gait + "\\gains.gns");
	std::vector<Gains *> gains = { m_torso_gains, m_ull_gains, m_url_gains, m_lll_gains, m_lrl_gains, m_lf_gains, m_rf_gains };
	for (std::vector<Gains *>::iterator it = gains.begin(); it != gains.end(); it++) {
		char buffer[100];
		sprintf_s(buffer, "%f, %f \n", (*it)->m_kp, (*it)->m_kd);
		std::cout << buffer;
		gains_file << buffer;
	}
	gains_file.close();
}

void WalkingController::SaveGains() {

	std::ofstream gains_file;
	gains_file.open("..\\..\\State Configurations\\gains.gns");
	std::vector<Gains *> gains = { m_torso_gains, m_ull_gains, m_url_gains, m_lll_gains, m_lrl_gains, m_lf_gains, m_rf_gains };
	for (std::vector<Gains *>::iterator it = gains.begin(); it != gains.end(); it++) {
		char buffer[100];
		sprintf_s(buffer, "%f, %f \n", (*it)->m_kp, (*it)->m_kd);
		std::cout << buffer;
		gains_file << buffer;
	}
	gains_file.close();
}

void WalkingController::SaveFeedback(std::string gait) {
	std::ofstream feedback_file;
	feedback_file.open("..\\..\\State Configurations\\Gaits\\" + gait + "\\feedbacks.fdbk");
	char buffer[100];
	sprintf_s(buffer, "%f, %f, %f, %f\n", *m_cd_1, *m_cv_1, *m_cd_2, *m_cv_2);
	std::cout << buffer;
	feedback_file << buffer;
	feedback_file.close();
}

void WalkingController::SaveFeedback() {
	std::ofstream feedback_file;
	feedback_file.open("..\\..\\State Configurations\\feedbacks.fdbk");
	char buffer[100];
	sprintf_s(buffer, "%f, %f, %f, %f\n", m_cd_1, m_cv_1, m_cd_2, m_cv_2);
	std::cout << buffer;
	feedback_file << buffer;
	feedback_file.close();
}

void WalkingController::SaveTime(std::string gait) {
	std::ofstream time_file;
	time_file.open("..\\..\\State Configurations\\Gaits\\" + gait + "\\stateTimes.tm");
	char buffer[100];
	sprintf_s(buffer, "%f\n", m_state_time);
	std::cout << buffer;
	time_file << buffer;
	time_file.close();
}

void WalkingController::SaveTime() {
	std::ofstream time_file;
	time_file.open("..\\..\\State Configurations\\stateTimes.tm");
	char buffer[100];
	sprintf_s(buffer, "%f\n", m_state_time);
	std::cout << buffer;
	time_file << buffer;
	time_file.close();
}

#pragma endregion FILE_IO


#pragma region WALKER_INTERACTION

void WalkingController::StateLoop() {

	switch (m_currentState)
	{
	case WALKING: {
		std::vector<float>torques = { 0, 0, 0, 0, 0, 0, 0 };

		if (m_torsoHasContacted)
		{
			
			m_torsoHasContacted = false;
			m_ragDollState = STATE_5;
			//printf("~*~*~*~*~*~*~*~*~*~ STATE 5 ~*~*~*~*~*~*~*~*~*~\n");
		}

		switch (m_ragDollState)
		{

		case STATE_0:
		{
			printf("~*~*~*~*~*~*~*~*~*~ STATE 0 ~*~*~*~*~*~*~*~*~*~\n");
			m_clock.reset();
			m_ragDollState = STATE_1;
			printf("~*~*~*~*~*~*~*~*~*~ STATE 1 ~*~*~*~*~*~*~*~*~*~\n");
		}
			break;
		case STATE_1: {
			if (!m_wait) {
				// Change the gait
				printf("Change Gait \n");
				m_torso_gains = m_torso_gains_tmp;
				m_ull_gains = m_ull_gains_tmp;
				m_url_gains = m_url_gains_tmp;
				m_lll_gains = m_lll_gains_tmp;
				m_lrl_gains = m_lrl_gains_tmp;
				m_lf_gains = m_lf_gains_tmp;
				m_rf_gains = m_rf_gains_tmp;

				m_state0 = m_state0_tmp;
				m_state1 = m_state1_tmp;
				m_state2 = m_state2_tmp;
				m_state3 = m_state3_tmp;
				m_state4 = m_state4_tmp;

				m_cd_1 = m_cd_1_tmp;
				m_cv_1 = m_cv_1_tmp;
				m_cd_2 = m_cd_2_tmp;
				m_cv_2 = m_cv_2_tmp;

				m_state_time = m_state_time_tmp;
				m_ragDoll->SetupGUIConfiguration();

				m_changeGait = false;
				m_wait = true;
			}		
			if (m_reset) {
				m_clock.reset();
				m_reset = false;
			}
			m_duration = m_clock.getTimeMilliseconds();
			if (m_duration >= m_state_time * 1000) {
				m_ragDollState = STATE_2;
				m_clock.reset();
				printf("~*~*~*~*~*~*~*~*~*~ STATE 2 ~*~*~*~*~*~*~*~*~*~\n");
			}
			else {
				// Compute torques for bodies
				torques = CalculateState1Torques();
			}
		}
			break;
		case STATE_2: {
			if (m_changeGait) {
				m_wait = false;
			}
			//torques = CalculateState2Torques();
			if (m_rightFootGroundHasContacted)
			{
				//printf("Right foot has contacted the floor. \n");
				// Contacted the floor
				m_ragDollState = STATE_3;
				m_rightFootGroundHasContacted = false;
				m_duration = 0;
				m_reset = true;
				printf("~*~*~*~*~*~*~*~*~*~ STATE 3 ~*~*~*~*~*~*~*~*~*~\n");
			}
			else {
				torques = CalculateState2Torques();
			}
		}
			break;
		case STATE_3: {
			if (m_changeGait) {
				m_wait = false;
			}
			if (m_reset) {
				m_clock.reset();
				m_reset = false;
			}
			m_duration = m_clock.getTimeMilliseconds();
			if (m_duration >= m_state_time * 1000) {
				m_ragDollState = STATE_4;
				m_clock.reset();
				printf("~*~*~*~*~*~*~*~*~*~ STATE 4 ~*~*~*~*~*~*~*~*~*~\n");
			}
			else {
				torques = CalculateState3Torques();
			}
		}
			break;
		case STATE_4: {
			if (m_changeGait) {
				m_wait = false;
			}
			if (m_leftFootGroundHasContacted)
			{
				// Contacted the floor
				m_ragDollState = STATE_1;
				m_leftFootGroundHasContacted = false;
				m_clock.reset();
				m_duration = 0;
				m_reset = true;
				printf("==============================\n~*~*~*~*~*~*~*~*~*~ STATE 1 ~*~*~*~*~*~*~*~*~*~\n");
			}
			else {
				torques = CalculateState4Torques();
			}
		}
			break;
		case STATE_5: {
			torques = { 0, 0, 0, 0, 0, 0, 0 };
		}
			break;
		default:
			break;
		}

		// Apply torque limits:
		//printf("Torques: ");
		for (std::vector<float>::iterator it = torques.begin(); it != torques.end(); ++it) {
			float *torqueValue = &(*it);
			if (*torqueValue > TORQUE_LIMIT)
			{
				*torqueValue = TORQUE_LIMIT;
			}
			if (*torqueValue < -TORQUE_LIMIT) {
				*torqueValue = -TORQUE_LIMIT;
			}

			//printf("%f, ", *torqueValue);
		}
		//printf("\n");
		// Apply torques to bodies
		m_ragDoll->ApplyTorqueOnUpperLeftLeg(torques.at(0));
		m_ragDoll->ApplyTorqueOnUpperRightLeg(torques.at(1));

		m_ragDoll->ApplyTorqueOnLowerLeftLeg(torques.at(2));
		m_ragDoll->ApplyTorqueOnLowerRightLeg(torques.at(3));

		m_ragDoll->ApplyTorqueOnLeftFoot(torques.at(4));
		m_ragDoll->ApplyTorqueOnRightFoot(torques.at(5));
	}
		break;
	case PAUSE:
		break;
	case RESET: {
		CalculateFeedbackSwingHip();
	}
		break;
	default:
		break;
	}

}

void WalkingController::PauseWalking(){
	m_currentState = PAUSE;
}

void WalkingController::InitiateWalking() {
	m_currentState = WALKING;
	m_ragDollState = STATE_0;
	m_reset = true;
}

void WalkingController::Reset(){
	m_torsoHasContacted = false;
	m_rightFootGroundHasContacted = false;
	m_leftFootGroundHasContacted = false;
	m_ragDollState = STATE_0;
	m_currentState = RESET;

}

void WalkingController::NotifyLeftFootGroundContact() {
	
	if (m_ragDollState == STATE_4) {
		m_leftFootGroundHasContacted = true;
	}
	else {
		m_leftFootGroundHasContacted = false;
	}

}

void WalkingController::NotifyRightFootGroundContact() {
	if (m_ragDollState == STATE_2) {
		m_rightFootGroundHasContacted = true;
	} else {
		m_rightFootGroundHasContacted = false;
	}
	
}

void WalkingController::NotifyTorsoGroundContact() {
	//printf("Torso contacted. \n");
	m_torsoHasContacted = true;
}

void WalkingController::ChangeGait(std::string gait) {

	printf("Gait is now %s\n", gait.c_str());
	m_currentGait = gait;
	// load the current gait
	std::vector<State *>states = m_GaitMap.find(gait)->second;
	std::vector<Gains *>gains = m_GainMap.find(gait)->second;
	std::vector<float>* feedback = &m_FdbkMap.find(gait)->second;
	float time = m_TmMap.find(gait)->second;
	printf("state time is %f\n", time);
	// interpolate (?)
	if (m_currentState == WALKING) {
		// Setting everything...
		m_state0_tmp = states.at(0);
		m_state1_tmp = states.at(1);
		m_state2_tmp = states.at(2);
		m_state3_tmp = states.at(3);
		m_state4_tmp = states.at(4);

		m_torso_gains_tmp = gains.at(0);
		m_ull_gains_tmp = gains.at(1);
		m_url_gains_tmp = gains.at(2);
		m_lll_gains_tmp = gains.at(3);
		m_lrl_gains_tmp = gains.at(4);
		m_lf_gains_tmp = gains.at(5);
		m_rf_gains_tmp = gains.at(6);

		m_cd_1_tmp = &feedback->at(0);
		m_cv_1_tmp = &feedback->at(1);
		m_cd_2_tmp = &feedback->at(2);
		m_cv_2_tmp = &feedback->at(3);

		m_state_time_tmp = time;

		m_changeGait = true;

	}
	else if (m_currentState == RESET) {
		// Setting everything...
		m_state0 = states.at(0);
		m_state1 = states.at(1);
		m_state2 = states.at(2);
		m_state3 = states.at(3);
		m_state4 = states.at(4);

		m_torso_gains = gains.at(0);
		m_ull_gains = gains.at(1);
		m_url_gains = gains.at(2);
		m_lll_gains = gains.at(3);
		m_lrl_gains = gains.at(4);
		m_lf_gains = gains.at(5);
		m_rf_gains = gains.at(6);

		m_cd_1 = &feedback->at(0);
		m_cv_1 = &feedback->at(1);
		m_cd_2 = &feedback->at(2);
		m_cv_2 = &feedback->at(3);

		m_state_time = time;
	}
}

#pragma endregion WALKER_INTERACTION

#pragma region GAINS

void WalkingController::SetTorsoGains(float kp, float kd){
	m_torso_gains->m_kp = kp;
	m_torso_gains->m_kd = kd;
}

void WalkingController::SetUpperLeftLegGains(float kp, float kd){
	m_ull_gains->m_kp = kp;
	m_ull_gains->m_kd = kd;
}

void WalkingController::SetUpperRightLegGains(float kp, float kd){
	m_url_gains->m_kp = kp;
	m_url_gains->m_kd = kd;
}

void WalkingController::SetLowerLeftLegGains(float kp, float kd){
	m_lll_gains->m_kp = kp;
	m_lll_gains->m_kd = kd;
}

void WalkingController::SetLowerRightLegGains(float kp, float kd){
	m_lrl_gains->m_kp = kp;
	m_lrl_gains->m_kd = kd;
}

void WalkingController::SetLeftFootGains(float kp, float kd){
	m_lf_gains->m_kp = kp;
	m_lf_gains->m_kd = kd;
}

void WalkingController::SetRightFootGains(float kp, float kd){
	m_rf_gains->m_kp = kp;
	m_rf_gains->m_kd = kd;
}

#pragma endregion GAINS

#pragma region STATES

void WalkingController::SetState1(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot){
	m_state1 = new State(torso, upperLeftLeg, upperRightLeg, lowerLeftLeg, lowerRightLeg, leftFoot, rightFoot);
}

void WalkingController::SetState2(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot){
	m_state2 = new State(torso, upperLeftLeg, upperRightLeg, lowerLeftLeg, lowerRightLeg, leftFoot, rightFoot);
}

void WalkingController::SetState3(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot){
	m_state3 = new State(torso, upperLeftLeg, upperRightLeg, lowerLeftLeg, lowerRightLeg, leftFoot, rightFoot);
}

void WalkingController::SetState4(float torso, float upperLeftLeg, float upperRightLeg, float lowerLeftLeg, float lowerRightLeg, float leftFoot, float rightFoot){
	
	m_state4 = new State(torso, upperLeftLeg, upperRightLeg, lowerLeftLeg, lowerRightLeg, leftFoot, rightFoot);

}

#pragma endregion STATES

#pragma region FEEDBACK

void WalkingController::SetFeedback1(float cd, float cv) {
	*m_cd_1 = cd;
	*m_cv_1 = cv;
}

void WalkingController::SetFeedback2(float cd, float cv) {
	*m_cd_2 = cd;
	*m_cv_2 = cv;
}

#pragma endregion FEEDBACK

#pragma region TIME

void WalkingController::SetStateTime(float time) {
	m_state_time = time;
	std::unordered_map<std::string, float>::iterator it = m_TmMap.find(m_currentGait);
	if (it != m_TmMap.end()) {
		it->second = m_state_time;
	}
}

#pragma endregion TIME

#pragma region CALCULATE_TORQUES

std::vector<float> WalkingController::CalculateState1Torques() {
	
	std::vector < float > torques{ 0 };

	/* ==================== Calculating torque for torso ======================= */
	float torsoTorque = CalculateTorqueForTorso(m_state1->m_torsoAngle, m_ragDoll->m_torso->GetOrientation(), m_ragDoll->m_torso->GetAngularVelocity());

	/* ==================== Calculating torque for Upper Legs ======================= */
	// SWING
	float feedbackTargetAngle = CalculateFeedbackSwingHip();
	float upperRightLegTorque = CalculateTorqueForUpperRightLeg(feedbackTargetAngle, m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_upperRightLeg->GetAngularVelocity());
	// STANCE
	//float upperLeftLegTorque = CalculateTorqueForUpperLeftLeg(m_state1->m_upperLeftLegAngle, m_ragDoll->m_upperLeftLeg->GetOrientation(), m_ragDoll->m_upperLeftLeg->GetAngularVelocity());
	float upperLeftLegTorque = - torsoTorque - upperRightLegTorque;

	/* ==================== Calculating torque for Lower Legs ======================= */
	float lowerLeftLegTorque = CalculateTorqueForLowerLeftLeg(m_ragDoll->m_upperLeftLeg->GetOrientation() - m_state1->m_lowerLeftLegAngle, m_ragDoll->m_lowerLeftLeg->GetOrientation(), m_ragDoll->m_lowerLeftLeg->GetAngularVelocity());
	float lowerRightLegTorque = CalculateTorqueForLowerRightLeg(m_ragDoll->m_upperRightLeg->GetOrientation() - m_state1->m_lowerRightLegAngle, m_ragDoll->m_lowerRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetAngularVelocity());
	
	//printf("****************************** \n url: orientation = %f\n lrl: orientation: %f \n", m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetOrientation());

	//printf("lrl: angle = %f, orientation = %f\n", m_state1->m_lowerRightLegAngle, m_ragDoll->m_upperRightLeg->GetOrientation() - m_ragDoll->m_lowerRightLeg->GetOrientation());

	/* ==================== Calculating torque for Feet ======================= */
	float leftFootTorque = CalculateTorqueForLeftFoot(
		m_ragDoll->m_lowerLeftLeg->GetOrientation() - m_state1->m_leftFootAngle,
		m_ragDoll->m_leftFoot->GetOrientation(),  
		m_ragDoll->m_leftFoot->GetAngularVelocity());
	float rightFootTorque = CalculateTorqueForRightFoot(
		m_ragDoll->m_lowerRightLeg->GetOrientation() - m_state1->m_rightFootAngle, 
		m_ragDoll->m_rightFoot->GetOrientation(), 
		m_ragDoll->m_rightFoot->GetAngularVelocity());

	//printf("rf: target angle = %f, orientation = %f\n", m_ragDoll->m_lowerRightLeg->GetOrientation() - m_state1->m_rightFootAngle, m_ragDoll->m_rightFoot->GetOrientation());

	torques = {upperLeftLegTorque - lowerLeftLegTorque, upperRightLegTorque - lowerRightLegTorque, lowerLeftLegTorque - leftFootTorque, lowerRightLegTorque - rightFootTorque, leftFootTorque, rightFootTorque };
	return torques;
}

std::vector<float> WalkingController::CalculateState2Torques() {

	std::vector < float > torques {0};
	
	/* ==================== Calculating torque for torso ======================= */
	float torsoTorque = CalculateTorqueForTorso(m_state2->m_torsoAngle, m_ragDoll->m_torso->GetOrientation(), m_ragDoll->m_torso->GetAngularVelocity());
	
	/* ==================== Calculating torque for Upper Legs ======================= */
	// SWING
	float feedbackTargetAngle = CalculateFeedbackSwingHip();
	float upperRightLegTorque = CalculateTorqueForUpperRightLeg(feedbackTargetAngle, m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_upperRightLeg->GetAngularVelocity());
	// STANCE
	float upperLeftLegTorque = -torsoTorque - upperRightLegTorque;

	/* ==================== Calculating torque for Lower Legs ======================= */
	float lowerLeftLegTorque = CalculateTorqueForLowerLeftLeg(m_ragDoll->m_upperLeftLeg->GetOrientation() - m_state2->m_lowerLeftLegAngle, m_ragDoll->m_lowerLeftLeg->GetOrientation(), m_ragDoll->m_lowerLeftLeg->GetAngularVelocity());
	float lowerRightLegTorque = CalculateTorqueForLowerRightLeg(m_ragDoll->m_upperRightLeg->GetOrientation() - m_state2->m_lowerRightLegAngle, m_ragDoll->m_lowerRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetAngularVelocity());

	//printf("****************************** \n url: orientation = %f\n lrl: orientation: %f \n", m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetOrientation());

	//printf("lrl: angle = %f, orientation = %f\n", m_state1->m_lowerRightLegAngle, m_ragDoll->m_upperRightLeg->GetOrientation() - m_ragDoll->m_lowerRightLeg->GetOrientation());

	/* ==================== Calculating torque for Feet ======================= */
	float leftFootTorque = CalculateTorqueForLeftFoot(
		m_ragDoll->m_lowerLeftLeg->GetOrientation() - m_state2->m_leftFootAngle,
		m_ragDoll->m_leftFoot->GetOrientation(),
		m_ragDoll->m_leftFoot->GetAngularVelocity());
	float rightFootTorque = CalculateTorqueForRightFoot(
		m_ragDoll->m_lowerRightLeg->GetOrientation() - m_state2->m_rightFootAngle,
		m_ragDoll->m_rightFoot->GetOrientation(),
		m_ragDoll->m_rightFoot->GetAngularVelocity());

	torques = { upperLeftLegTorque - lowerLeftLegTorque, upperRightLegTorque - lowerRightLegTorque, lowerLeftLegTorque - leftFootTorque, lowerRightLegTorque - rightFootTorque, leftFootTorque, rightFootTorque };
	return torques;
}

std::vector<float> WalkingController::CalculateState3Torques() {

	std::vector < float > torques{ 0 };

	/* ==================== Calculating torque for torso ======================= */
	float torsoTorque = CalculateTorqueForTorso(m_state3->m_torsoAngle, m_ragDoll->m_torso->GetOrientation(), m_ragDoll->m_torso->GetAngularVelocity());
	//printf("torso: desired = %f, current = %f , torque = %f\n", m_state3->m_torsoAngle, m_ragDoll->m_torso->GetOrientation(), torsoTorque);
	/* ==================== Calculating torques for upper legs ================= */
	// SWING
	float feedbackTargetAngle = CalculateFeedbackSwingHip();
	float upperLeftLegTorque = CalculateTorqueForUpperLeftLeg(feedbackTargetAngle, m_ragDoll->m_upperLeftLeg->GetOrientation(), m_ragDoll->m_upperLeftLeg->GetAngularVelocity());
	//printf("ULL: desired = %f, current = %f, torque = %f \n", feedbackTargetAngle, m_ragDoll->m_upperLeftLeg->GetOrientation(), upperLeftLegTorque);
	// STANCE
	float upperRightLegTorque = -torsoTorque - upperLeftLegTorque;
	//printf("URL torque = %f\n", upperRightLegTorque);

	/* ==================== Calculating torque for Lower Legs ======================= */
	float lowerLeftLegTorque = CalculateTorqueForLowerLeftLeg(m_ragDoll->m_upperLeftLeg->GetOrientation() - m_state3->m_lowerLeftLegAngle, m_ragDoll->m_lowerLeftLeg->GetOrientation(), m_ragDoll->m_lowerLeftLeg->GetAngularVelocity());
	float lowerRightLegTorque = CalculateTorqueForLowerRightLeg(m_ragDoll->m_upperRightLeg->GetOrientation() - m_state3->m_lowerRightLegAngle, m_ragDoll->m_lowerRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetAngularVelocity());

	//printf("****************************** \n url: orientation = %f\n lrl: orientation: %f \n", m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetOrientation());

	//printf("lrl: angle = %f, orientation = %f\n", m_state1->m_lowerRightLegAngle, m_ragDoll->m_upperRightLeg->GetOrientation() - m_ragDoll->m_lowerRightLeg->GetOrientation());

	/* ==================== Calculating torque for Feet ======================= */
	float leftFootTorque = CalculateTorqueForLeftFoot(
		m_ragDoll->m_lowerLeftLeg->GetOrientation() - m_state3->m_leftFootAngle,
		m_ragDoll->m_leftFoot->GetOrientation(),
		m_ragDoll->m_leftFoot->GetAngularVelocity());
	float rightFootTorque = CalculateTorqueForRightFoot(
		m_ragDoll->m_lowerRightLeg->GetOrientation() - m_state3->m_rightFootAngle,
		m_ragDoll->m_rightFoot->GetOrientation(),
		m_ragDoll->m_rightFoot->GetAngularVelocity());

	torques = { upperLeftLegTorque - lowerLeftLegTorque, upperRightLegTorque - lowerRightLegTorque, lowerLeftLegTorque - leftFootTorque, lowerRightLegTorque - rightFootTorque, leftFootTorque, rightFootTorque };
	return torques;
}

std::vector<float> WalkingController::CalculateState4Torques() {
	
	std::vector < float > torques{ 0 };

	/* =================== Calculating torque for torso ======================= */
	float torsoTorque = CalculateTorqueForTorso(m_state4->m_torsoAngle, m_ragDoll->m_torso->GetOrientation(), m_ragDoll->m_torso->GetAngularVelocity());

	/* ==================== Calculating torque for Upper Legs ======================= */
	// SWING
	float feedbackTargetAngle = CalculateFeedbackSwingHip();
	float upperLeftLegTorque = CalculateTorqueForUpperLeftLeg(feedbackTargetAngle, m_ragDoll->m_upperLeftLeg->GetOrientation(), m_ragDoll->m_upperLeftLeg->GetAngularVelocity());

	// STANCE
	float upperRightLegTorque = -torsoTorque - upperLeftLegTorque;

	/* ==================== Calculating torque for Lower Legs ======================= */
	float lowerLeftLegTorque = CalculateTorqueForLowerLeftLeg(m_ragDoll->m_upperLeftLeg->GetOrientation() - m_state4->m_lowerLeftLegAngle, m_ragDoll->m_lowerLeftLeg->GetOrientation(), m_ragDoll->m_lowerLeftLeg->GetAngularVelocity());
	float lowerRightLegTorque = CalculateTorqueForLowerRightLeg(m_ragDoll->m_upperRightLeg->GetOrientation() - m_state4->m_lowerRightLegAngle, m_ragDoll->m_lowerRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetAngularVelocity());

	//printf("****************************** \n url: orientation = %f\n lrl: orientation: %f \n", m_ragDoll->m_upperRightLeg->GetOrientation(), m_ragDoll->m_lowerRightLeg->GetOrientation());

	//printf("lrl: angle = %f, orientation = %f\n", m_state1->m_lowerRightLegAngle, m_ragDoll->m_upperRightLeg->GetOrientation() - m_ragDoll->m_lowerRightLeg->GetOrientation());

	/* ==================== Calculating torque for Feet ======================= */
	float leftFootTorque = CalculateTorqueForLeftFoot(
		m_ragDoll->m_lowerLeftLeg->GetOrientation() - m_state4->m_leftFootAngle,
		m_ragDoll->m_leftFoot->GetOrientation(),
		m_ragDoll->m_leftFoot->GetAngularVelocity());
	float rightFootTorque = CalculateTorqueForRightFoot(
		m_ragDoll->m_lowerRightLeg->GetOrientation() - m_state4->m_rightFootAngle,
		m_ragDoll->m_rightFoot->GetOrientation(),
		m_ragDoll->m_rightFoot->GetAngularVelocity());

	//std::cout << "Torques Before (ULL-swing: " << upperLeftLegTorque << ", URL-stance: " << upperRightLegTorque << ", LLL: " << lowerLeftLegTorque << ", LRL: " << lowerRightLegTorque << ", LF: " << leftFootTorque << ", RF: " << rightFootTorque << std::endl;
	//std::cout << "Torques After (ULL-swing: " << upperLeftLegTorque - lowerLeftLegTorque << ", URL-stance: " << upperRightLegTorque - lowerRightLegTorque << ", LLL: " << lowerLeftLegTorque - leftFootTorque << ", LRL: " << lowerRightLegTorque  - rightFootTorque << ", LF: " << leftFootTorque << ", RF: " << rightFootTorque << std::endl;
	torques = { upperLeftLegTorque - lowerLeftLegTorque, upperRightLegTorque - lowerRightLegTorque, lowerLeftLegTorque - leftFootTorque, lowerRightLegTorque - rightFootTorque, leftFootTorque, rightFootTorque };
	return torques;
}

float WalkingController::CalculateFeedbackSwingHip() {

	// Swing hip changes between states.
	GameObject *swingHipBody;
	float targetAngle = 0.0f;
	float distance = 0.0f;
	float velocity = m_ragDoll->m_torso->GetRigidBody()->getVelocityInLocalPoint(btVector3(0, -torso_height/2, 0)).x();
	float cd = 0.0f, cv = 0.0f;
	btVector3 stanceAnkle(0, 0, 0);
	btVector3 hipPosition = m_ragDoll->m_torso->GetCOMPosition()
		+ btVector3(sin(Constants::GetInstance().DegreesToRadians(m_ragDoll->m_torso->GetOrientation())) * torso_height / 2,
		-cos(Constants::GetInstance().DegreesToRadians(m_ragDoll->m_torso->GetOrientation())) * torso_height / 2,
		0);
	/*btVector3 hipPosition = m_ragDoll->m_upperRightLeg->GetCOMPosition()
		+ btVector3(cos(Constants::GetInstance().DegreesToRadians(m_ragDoll->m_upperRightLeg->GetOrientation())) * upper_leg_height / 2,
		+sin(Constants::GetInstance().DegreesToRadians(m_ragDoll->m_upperRightLeg->GetOrientation())) * upper_leg_height / 2,
		0);*/
	switch (m_ragDollState)
	{
	case STATE_0: {
		swingHipBody = m_ragDoll->m_upperRightLeg;
		targetAngle = m_state1->m_upperRightLegAngle;
		// Calculate the distance to stance ankle
		float lllAngle = Constants::GetInstance().DegreesToRadians(m_ragDoll->m_lowerLeftLeg->GetOrientation());
		stanceAnkle = m_ragDoll->m_lowerLeftLeg->GetCOMPosition() + btVector3(sin(lllAngle) * lower_leg_height / 2, -cos(lllAngle) * lower_leg_height / 2, 0);
	}
		break;
	case STATE_1: {
		// RIGHT Leg Swing
		swingHipBody = m_ragDoll->m_upperRightLeg;
		targetAngle = m_state1->m_upperRightLegAngle;
		// Calculate the distance to stance ankle
		//printf("lower left leg angle = %f\n", m_ragDoll->m_lowerLeftLeg->GetOrientation());
		float lllAngle = Constants::GetInstance().DegreesToRadians(m_ragDoll->m_lowerLeftLeg->GetOrientation());
		stanceAnkle = m_ragDoll->m_lowerLeftLeg->GetCOMPosition() + btVector3(sin(lllAngle) * lower_leg_height / 2, -cos(lllAngle) * lower_leg_height / 2, 0);

		cd = *m_cd_1;
		cv = *m_cv_1;
	}
		break;
	case STATE_2: {
		// RIGHT Leg Swing
		swingHipBody = m_ragDoll->m_upperRightLeg;
		targetAngle = m_state2->m_upperRightLegAngle;
		// Calculate the distance to stance ankle
		float lllAngle = Constants::GetInstance().DegreesToRadians(m_ragDoll->m_lowerLeftLeg->GetOrientation());
		stanceAnkle = m_ragDoll->m_lowerLeftLeg->GetCOMPosition() + btVector3(sin(lllAngle) * lower_leg_height / 2, -cos(lllAngle) * lower_leg_height / 2, 0);
		// Approximate center of mass x position to be the same as torso x position
		
		cd = *m_cd_2;
		cv = *m_cv_2;
	}
		break;
	case STATE_3: {
		// LEFT leg Swing
		swingHipBody = m_ragDoll->m_upperLeftLeg;
		targetAngle = m_state3->m_upperLeftLegAngle;
		// Calculate the distance to stance ankle
		float lrlAngle = Constants::GetInstance().DegreesToRadians(m_ragDoll->m_lowerRightLeg->GetOrientation());
		stanceAnkle = m_ragDoll->m_lowerRightLeg->GetCOMPosition() + btVector3(sin(lrlAngle) * lower_leg_height / 2, -cos(lrlAngle) * lower_leg_height / 2, 0);

		cd = *m_cd_1;
		cv = *m_cv_1;
	}
		break;
	case STATE_4: {
		// LEFT leg Swing
		swingHipBody = m_ragDoll->m_upperLeftLeg;
		targetAngle = m_state4->m_upperLeftLegAngle;

		// Calculate the distance to stance ankle
		float lrlAngle = Constants::GetInstance().DegreesToRadians(m_ragDoll->m_lowerRightLeg->GetOrientation());
		stanceAnkle = m_ragDoll->m_lowerRightLeg->GetCOMPosition() + btVector3(sin(lrlAngle) * lower_leg_height / 2, -cos(lrlAngle) * lower_leg_height / 2, 0);

		cd = *m_cd_2;
		cv = *m_cv_2;
	}
		break;
	default:
		break;
	}

	distance = hipPosition.x() - stanceAnkle.x();

	m_stanceAnklePosition = stanceAnkle;
	m_COMPosition = hipPosition;

	return targetAngle + cd * distance + cv * velocity;

}

float WalkingController::CalculateTorqueForTorso(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_torso_gains->m_kp, m_torso_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForUpperLeftLeg(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_ull_gains->m_kp, m_ull_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForUpperRightLeg(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_url_gains->m_kp, m_url_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForLowerLeftLeg(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_lll_gains->m_kp, m_lll_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForLowerRightLeg(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_lrl_gains->m_kp, m_lrl_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForLeftFoot(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_lf_gains->m_kp, m_lf_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorqueForRightFoot(float targetPosition, float currentPosition, float currentVelocity) {
	return CalculateTorque(m_rf_gains->m_kp, m_rf_gains->m_kd, targetPosition, currentPosition, currentVelocity);
}

float WalkingController::CalculateTorque(float kp, float kd, float targetPosition, float currentPosition, float velocity) {

	return kp * Constants::GetInstance().DegreesToRadians(targetPosition - currentPosition) - kd * Constants::GetInstance().DegreesToRadians(velocity);

}

#pragma endregion CALCULATE_TORQUES