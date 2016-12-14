#include "stdafx.h"
#include "RagDoll.h"

#include "State.h"	
#include "Gains.h"

#include "BulletCollision\CollisionShapes\btBox2dShape.h"
#include "BulletDynamics\Dynamics\btDynamicsWorld.h"
#include "ContactManager.h"

#include "WalkingController.h"

#include "BulletOpenGLApplication.h"

#include <utility>

static RagDoll *m_ragDoll;

enum ControlIDs {
	TORSO_KP, TORSO_KD = 0,
	UPPER_R_LEG_KP, UPPER_R_LEG_KD,
	UPPER_L_LEG_KP, UPPER_L_LEG_KD,
	LOWER_R_LEG_KP, LOWER_R_LEG_KD,
	LOWER_L_LEG_KP, LOWER_L_LEG_KD,
	R_FOOT_KP, R_FOOT_KD,
	L_FOOT_KP, L_FOOT_KD
};

enum StateIDs {
	TORSO_ANGLE = 13,
	ULL_ANGLE,
	URL_ANGLE,
	LLL_ANGLE,
	LRL_ANGLE,
	LF_ANGLE,
	RF_ANGLE
};

enum Button_IDs {
	RESET_BUTTON = 20,
	PAUSE_BUTTON,
	START_BUTTON,
	SAVESTATES_BUTTON,
	SAVEGAINS_BUTTON,
	SAVEFEEDBACK_BUTTON,
	SAVETIME_BUTTON
};

#pragma region RagDoll

RagDoll::RagDoll(BulletOpenGLApplication *app, bool useCustomContact)
{
	m_app = app;
	m_useCustomContact = useCustomContact;
}

RagDoll::~RagDoll()
{
}

void RagDoll::InitializeRagDoll(float halfGroundHeight, int window_id) {

	m_ragDoll = this;

	//randomBox = Create2DBox(btVector3(0.5f, 0.5f, 0.0f), 5.0f, btVector3(1.0f, 1.0f, 0.0f), btVector3(5.0f, 3.5f, 0.0f));
	//ContactManager::GetInstance().AddObjectForCollision(randomBox, 10);

	m_origTorsoPosition = btVector3(0.0f, 0.0f + halfGroundHeight + foot_height / 2 + lower_leg_height + upper_leg_height + torso_height/2, 0);

	CreateRagDoll(m_origTorsoPosition);

	m_WalkingController = new WalkingController(this);
	std::vector<std::string> gaits = m_WalkingController->GetGaits();
	int ind = 0;
	for (std::vector<std::string>::iterator it = gaits.begin(); it != gaits.end(); ++it) {
		std::string gait_name = *it;
		m_gaits.push_back(gait_name);
	}
	m_WalkingController->ChangeGait("Walk"); // Assume Walk gait exists

	m_previousState = 0;

	// Create GUI
	CreateRagDollGUI(window_id);

	// Setup GUI with configurations
	SetupGUIConfiguration();

	Reset();

	GameObject::DisableDeactivation(m_bodies);

	
	if (m_useCustomContact) {
		m_leftFootCollider = std::unique_ptr<ColliderObject> (ContactManager::GetInstance().AddObjectForCollision(m_leftFoot, 20));
		m_rightFootCollider = std::unique_ptr<ColliderObject> (ContactManager::GetInstance().AddObjectForCollision(m_rightFoot, 20));
		//m_torsoCollider = std::unique_ptr<ColliderObject>(ContactManager::GetInstance().AddObjectForCollision(m_torso));
	}

}

void RagDoll::RagDollCollision(btScalar timestep, btDynamicsWorld *world, std::deque<GameObject *> & grounds) {

	if (ContactManager::GetInstance().m_beingUsed) {

		ContactManager::GetInstance().Update(timestep);

		if (m_disabled) {
			ContactManager::GetInstance().Reset();
		}

		std::set<GameObject *> lfGOs;
		std::set<GameObject *> rfGOs;

		ContactManager::GetInstance().GetCollidingGameObjectsForObject(m_leftFootCollider.get(), lfGOs);
		ContactManager::GetInstance().GetCollidingGameObjectsForObject(m_rightFootCollider.get(), rfGOs);

		for (auto const & ground : grounds) {
			if (!lfGOs.empty()) {
				if (lfGOs.find(ground) != lfGOs.end()) {
					//printf("Notify left foot. \n");
					m_WalkingController->NotifyLeftFootGroundContact();
				}
			}
			if (!rfGOs.empty()) {
				if (!rfGOs.empty() && rfGOs.find(ground) != rfGOs.end()) {
					//printf("Notify right foot. \n");
					m_WalkingController->NotifyRightFootGroundContact();
				}
			}
		}
		int numManifolds = world->getDispatcher()->getNumManifolds();
		for (int i = 0; i < numManifolds; i++) {
				btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);

				btCollisionObject* obA = const_cast<btCollisionObject*>(contactManifold->getBody0());
				btCollisionObject* obB = const_cast<btCollisionObject*>(contactManifold->getBody1());

				for (int j = 0; j < contactManifold->getNumContacts(); j++)   {
					btManifoldPoint& pt = contactManifold->getContactPoint(j);

					for (auto const & ground : grounds) {
						if (pt.m_distance1 < 0) {
							if ((obA->getUserPointer() == m_torso && obB->getUserPointer() == ground) || (obA->getUserPointer() == ground && obB->getUserPointer() == m_torso)) {
								//printf(">>>>>>>>>>>>>>>>>>>>>> Collision with right foot to ground detected. <<<<<<<<<<<<<<<<<<<<<< \n");
								GameObject::ClearForces({ m_torso });
								GameObject::ClearVelocities({ m_torso });
								m_WalkingController->NotifyTorsoGroundContact();

								try {
									m_SimulationEnd();
								}
								catch (const std::bad_function_call& e) {
									//std::cout << e.what() << '\n';
								}

								break;
							}
						}
					}
				}
			}
	}
	else {
		// Use Default collision detection
		int numManifolds = world->getDispatcher()->getNumManifolds();
		for (int i = 0; i < numManifolds; i++)
		{
			btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);

			btCollisionObject* obA = const_cast<btCollisionObject*>(contactManifold->getBody0());
			btCollisionObject* obB = const_cast<btCollisionObject*>(contactManifold->getBody1());

			for (int j = 0; j < contactManifold->getNumContacts(); j++)   {
				btManifoldPoint& pt = contactManifold->getContactPoint(j);

				for (auto const & ground : grounds) {
					if (pt.m_distance1 < 0) {
						// Valid contact point
						if ((obA->getUserPointer() == m_leftFoot && obB->getUserPointer() == ground) || (obA->getUserPointer() == ground && obB->getUserPointer() == m_leftFoot)) {
							//printf(">>>>>>>>>>>>>>>>>>>>>> Collision with left foot to ground detected. <<<<<<<<<<<<<<<<<<<<<< \n");
							m_WalkingController->NotifyLeftFootGroundContact();
							break;
						}

						if ((obA->getUserPointer() == m_rightFoot && obB->getUserPointer() == ground) || (obA->getUserPointer() == ground && obB->getUserPointer() == m_rightFoot)) {
							//printf(">>>>>>>>>>>>>>>>>>>>>> Collision with right foot to ground detected. <<<<<<<<<<<<<<<<<<<<<< \n");
							m_WalkingController->NotifyRightFootGroundContact();
							break;
						}
						if ((obA->getUserPointer() == m_torso && obB->getUserPointer() == ground) || (obA->getUserPointer() == ground && obB->getUserPointer() == m_torso)) {
							//printf(">>>>>>>>>>>>>>>>>>>>>> Collision with right foot to ground detected. <<<<<<<<<<<<<<<<<<<<<< \n");
							GameObject::ClearForces({ m_torso });
							GameObject::ClearVelocities({ m_torso });
							m_WalkingController->NotifyTorsoGroundContact();

							try {
								m_SimulationEnd();
							}
							catch (const std::bad_function_call& e) {
								//std::cout << e.what() << '\n';
							}

							break;
						}
					}
				}
			}
		}
	}
}

void RagDoll::Loop()	{
	m_WalkingController->StateLoop();

	if (m_disabled) {
		ContactManager::GetInstance().Reset();
	}

}

void RagDoll::AddCollisionWithGround(GameObject *ground) {

	ContactManager::GetInstance().AddCollisionPair(m_leftFoot, ground);
	ContactManager::GetInstance().AddCollisionPair(m_rightFoot, ground);
	//ContactManager::GetInstance().AddCollisionPair(m_torso, ground);

}

void RagDoll::RemoveCollisionWithGround(GameObject *ground)	{

	ContactManager::GetInstance().RemoveCollisionPair(m_leftFoot, ground);
	ContactManager::GetInstance().RemoveCollisionPair(m_rightFoot, ground);
	//ContactManager::GetInstance().RemoveCollisionPair(m_torso, ground);

}

void RagDoll::ConfigureContactModel() {
	ContactManager::GetInstance().AddObjectForCollision(m_rightFoot);
	ContactManager::GetInstance().AddObjectForCollision(m_leftFoot);
}

void RagDoll::CreateRagDoll(const btVector3 &position) {

	// Create a torso centered at the position
	btVector3 torsoHalfSize(torso_width / 2, torso_height / 2, 0.04);
	btVector3 ulHalfSize = btVector3(upper_leg_width / 2, upper_leg_height / 2, 0.04f);
	btVector3 llHalfSize = btVector3(lower_leg_width / 2, lower_leg_height / 2, 0.04f);
	btVector3 fHalfSize = btVector3(foot_width / 2, foot_height / 2, 0.04f);

	// Create RIGHT LEG	
	m_upperRightLeg = Create2DBox(ulHalfSize, upper_leg_mass, btVector3(0 / 256.0, 153 / 256.0, 0 / 256.0), position + btVector3(0, 0, 0.15), "upper right leg"); // Green
	m_rightFoot = Create2DBox(fHalfSize, feet_mass, btVector3(153 / 256.0, 0 / 256.0, 153 / 256.0), position + btVector3(0, 0, 0.45), "Right foot"); // purple
	m_lowerRightLeg = Create2DBox(llHalfSize, lower_leg_mass, btVector3(255 / 256.0, 102 / 256.0, 0 / 256.0), position + btVector3(0, 0, 0.3), "Lower right leg"); // Orange

	m_torso = Create2DBox(torsoHalfSize, torso_mass, btVector3(0, 51 / 256.0, 102 / 256.0), position, "torso"); // Blue

	// Create LEFT LEG
	m_upperLeftLeg = Create2DBox(ulHalfSize, upper_leg_mass, btVector3(255 / 256.0, 102 / 256.0, 255 / 256.0), position + btVector3(0, 0, -0.15), "Upper right leg"); // Pink
	m_leftFoot = Create2DBox(fHalfSize, feet_mass, btVector3(0 / 256.0, 255 / 256.0, 255 / 256.0), position + btVector3(0, 0, -0.451), "Left foot"); // aqua blue
	m_lowerLeftLeg = Create2DBox(llHalfSize, lower_leg_mass, btVector3(250 / 256.0, 250 / 256.0, 10 / 256.0), position + btVector3(0, 0, -0.3), "lower left leg"); // Yellow

	AddHinges();

	m_bodies = { m_torso, m_upperLeftLeg, m_upperRightLeg, m_lowerLeftLeg, m_lowerRightLeg, m_leftFoot, m_rightFoot };

	//m_bodies = { m_torso };

	

}

void RagDoll::AddHinges() {

	// Connect torso to upper legs
	m_torso_ulLeg = m_app->AddHingeConstraint(m_torso, m_upperLeftLeg, btVector3(0, -torso_height / 2, 0), btVector3(0, upper_leg_height / 2, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_TORSO_ULL_LOW), Constants::GetInstance().DegreesToRadians(HINGE_TORSO_ULL_HIGH));
	m_torso_urLeg = m_app->AddHingeConstraint(m_torso, m_upperRightLeg, btVector3(0, -torso_height / 2, 0), btVector3(0, upper_leg_height / 2, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_TORSO_ULL_LOW), Constants::GetInstance().DegreesToRadians(HINGE_TORSO_URL_HIGH));

	// Connect upper legs to lower legs
	m_ulLeg_llLeg = m_app->AddHingeConstraint(m_upperLeftLeg, m_lowerLeftLeg, btVector3(0, -upper_leg_height / 2, 0), btVector3(0, lower_leg_height / 2, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_ULL_LLL_LOW), Constants::GetInstance().DegreesToRadians(HINGE_ULL_LLL_HIGH));
	m_urLeg_lrLeg = m_app->AddHingeConstraint(m_upperRightLeg, m_lowerRightLeg, btVector3(0, -upper_leg_height / 2, 0), btVector3(0, lower_leg_height / 2, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_URL_LRL_LOW), Constants::GetInstance().DegreesToRadians(HINGE_URL_LRL_HIGH));

	// Connect feet to lower legs
	m_llLeg_lFoot = m_app->AddHingeConstraint(m_lowerLeftLeg, m_leftFoot, btVector3(0, -lower_leg_height / 2, 0), btVector3(-(foot_width) / 4, 0, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_LLL_LF_LOW), Constants::GetInstance().DegreesToRadians(HINGE_LLL_LF_HIGH));
	m_lrLeg_rFoot = m_app->AddHingeConstraint(m_lowerRightLeg, m_rightFoot, btVector3(0, -lower_leg_height / 2, 0), btVector3(-(foot_width) / 4, 0, 0), btVector3(0, 0, 1), btVector3(0, 0, 1), Constants::GetInstance().DegreesToRadians(HINGE_LRL_RF_LOW), Constants::GetInstance().DegreesToRadians(HINGE_LRL_RF_HIGH));

}

GameObject *RagDoll::Create2DBox(const btVector3 &halfSize, float mass, const btVector3 &color, const btVector3 &position, std::string name) {
	// Multiply size by scaling factor
	GameObject *aBox = m_app->CreateGameObject(new btBox2dShape(halfSize), mass, color, position, name);
	return aBox;
}

void RagDoll::Idle() {
	m_app->Idle();
}

State *RagDoll::GetState(int state) {
	switch (state)
	{
	case 0:
		return m_WalkingController->m_state0;
		break;
	case 1:
		return m_WalkingController->m_state1;
		break;
	case 2:
		return m_WalkingController->m_state2;
		break;
	case 3:
		return m_WalkingController->m_state3;
		break;
	case 4:
		return m_WalkingController->m_state4;
		break;
	default:
		break;
	}
}

void RagDoll::UpdateRagDoll() {

	State *state = NULL;

	state = GetState(m_currentState);

	float torsoAngle = Constants::GetInstance().DegreesToRadians(state->m_torsoAngle);
	float ullAngle = Constants::GetInstance().DegreesToRadians(state->m_upperLeftLegAngle);
	float urlAngle = Constants::GetInstance().DegreesToRadians(state->m_upperRightLegAngle);
	// Local coordinates convert to world coordinates
	float lllAngle = Constants::GetInstance().DegreesToRadians(state->m_upperLeftLegAngle - state->m_lowerLeftLegAngle);
	float lrlAngle = Constants::GetInstance().DegreesToRadians(state->m_upperRightLegAngle - state->m_lowerRightLegAngle);

	float lfAngle = lllAngle - Constants::GetInstance().DegreesToRadians(state->m_leftFootAngle);
	float rfAngle = lrlAngle - Constants::GetInstance().DegreesToRadians(state->m_rightFootAngle);

	// Blue

	m_torso->Reposition(
		m_origTorsoPosition + btVector3(-(torso_height / 2) * sin(torsoAngle), (torso_height / 2) * (cos(torsoAngle) - 1), 0),
		btQuaternion(btVector3(0, 0, 1), torsoAngle));
	Debug("Torso COM (" << m_torso->GetCOMPosition().x() << ", " << m_torso->GetCOMPosition().y() << ", " << m_torso->GetCOMPosition().z() << ")");


	btVector3 hipPosition = m_origTorsoPosition + btVector3(0, -torso_height / 2, 0);  // Hip stays constant

	// GREEN
	m_upperRightLeg->Reposition(
		hipPosition + btVector3(0, -upper_leg_height / 2, 0.150)
		+ btVector3((upper_leg_height / 2) * sin(urlAngle), (upper_leg_height / 2)*(1 - cos(urlAngle)), 0),
		btQuaternion(btVector3(0, 0, 1), urlAngle));
	Debug("URL COM (" << m_upperRightLeg->GetCOMPosition().x() << ", " << m_upperRightLeg->GetCOMPosition().y() << ", " << m_upperRightLeg->GetCOMPosition().z(), ")");

	// PINK
	m_upperLeftLeg->Reposition(
		hipPosition + btVector3(0, -upper_leg_height / 2, -0.150)
		+ btVector3((upper_leg_height / 2) * sin(ullAngle), (upper_leg_height / 2)*(1 - cos(ullAngle)), 0),
		btQuaternion(btVector3(0, 0, 1), ullAngle));
	Debug("ULL COM (" << m_upperLeftLeg->GetCOMPosition().x() << ", " << m_upperLeftLeg->GetCOMPosition().y() << ", " << m_upperLeftLeg->GetCOMPosition().z() << ")");

	btVector3 upperRightLegBottomPoint = m_upperRightLeg->GetCOMPosition() + btVector3(sin(urlAngle) * upper_leg_height / 2, -cos(urlAngle) * upper_leg_height / 2, 0);

	// ORANGE
	m_lowerRightLeg->Reposition(
		upperRightLegBottomPoint + btVector3((lower_leg_height / 2) * sin(lrlAngle), -(lower_leg_height / 2)*(cos(lrlAngle)), 0.150),
		btQuaternion(btVector3(0, 0, 1), lrlAngle));
	Debug("LRL COM (" << m_lowerRightLeg->GetCOMPosition().x() << ", " << m_lowerRightLeg->GetCOMPosition().y() << ", " << m_lowerRightLeg->GetCOMPosition().z(), ")");

	btVector3 upperLeftLegBottomPoint = m_upperLeftLeg->GetCOMPosition() + btVector3(sin(ullAngle) * upper_leg_height / 2, -cos(ullAngle) * upper_leg_height / 2, 0);

	m_lowerLeftLeg->Reposition(
		upperLeftLegBottomPoint + btVector3((lower_leg_height / 2) * sin(lllAngle), -(lower_leg_height / 2) * cos(lllAngle), -0.150),
		btQuaternion(btVector3(0, 0, 1), lllAngle));

	Debug("LLL COM (" << m_lowerLeftLeg->GetCOMPosition().x() << ", " << m_lowerLeftLeg->GetCOMPosition().y() << ", " << m_lowerLeftLeg->GetCOMPosition().z() << ")");

	btVector3 lowerRightLegBottomPoint = m_lowerRightLeg->GetCOMPosition() + btVector3(sin(lrlAngle) * lower_leg_height / 2, -cos(lrlAngle) * lower_leg_height / 2, 0);
	// PURPLE
	m_rightFoot->Reposition(
		lowerRightLegBottomPoint + btVector3(cos(rfAngle) * foot_width / 4, sin(rfAngle) * foot_width / 4, 0.150),
		btQuaternion(btVector3(0, 0, 1), rfAngle));
	Debug("RF COM (" << m_rightFoot->GetCOMPosition().x() << ", " << m_rightFoot->GetCOMPosition().y() << ", " << m_rightFoot->GetCOMPosition().z() << ")");

	btVector3 lowerLeftLegBottomPoint = m_lowerLeftLeg->GetCOMPosition() + btVector3(sin(lllAngle) * lower_leg_height / 2, -cos(lllAngle) * lower_leg_height / 2, 0);

	m_leftFoot->Reposition(
		lowerLeftLegBottomPoint + btVector3(cos(lfAngle) * foot_width / 4, sin(lfAngle) * foot_width / 4, -0.150),
		btQuaternion(btVector3(0, 0, 1), lfAngle));

	Debug("LF COM (" << m_leftFoot->GetCOMPosition().x() << ", " << m_leftFoot->GetCOMPosition().y() << ", " << m_leftFoot->GetCOMPosition().z() << ")");

}

btVector3 RagDoll::GetLocation() {
	return m_torso->GetCOMPosition();
}

std::vector<float> RagDoll::GetOrientationsAndAngularVelocities() {

	return std::vector<float> {
		m_torso->GetOrientation(),
		m_torso->GetAngularVelocity(),
		m_upperRightLeg->GetOrientation(),
		m_upperRightLeg->GetAngularVelocity(),
		m_upperLeftLeg->GetOrientation(),
		m_upperLeftLeg->GetAngularVelocity(),
		m_lowerRightLeg->GetOrientation(),
		m_lowerRightLeg->GetAngularVelocity(),
		m_lowerLeftLeg->GetOrientation(),
		m_lowerLeftLeg->GetAngularVelocity(),
		m_rightFoot->GetOrientation(),
		m_rightFoot->GetAngularVelocity(),
		m_leftFoot->GetOrientation(),
		m_leftFoot->GetAngularVelocity()
	};
}

btVector3 RagDoll::GetTorsoLinearVelocity() {
	btVector3 torso_velocity = m_torso->GetRigidBody()->getLinearVelocity();
	//printf("torso vel x: %f, y: %f \n", torso_velocity.x(), torso_velocity.y());
	return torso_velocity;
}

std::pair<std::vector<btVector3>, std::vector<btVector3>> RagDoll::GetContactForces() {
	return std::make_pair(m_rightFootCollider->GetForcesOnVertexes(), m_leftFootCollider->GetForcesOnVertexes());
}

#pragma endregion RagDoll

#pragma region GUI

void RagDoll::CreateRagDollGUI(int window_id) {

	// Setup
	GLUI_Master.set_glutIdleFunc(RagDollIdle);
	m_glui_window = GLUI_Master.create_glui("Rag Doll Controls", 5, 0);
	m_glui_window->set_main_gfx_window(window_id);

	// Controls
	/*===================================== GAINS =========================================*/
	GLUI_Panel *gains_panel = m_glui_window->add_panel("Gains");
	m_glui_window->add_statictext_to_panel(gains_panel, "Torso");
	m_torso_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, TORSO_KP, (GLUI_Update_CB)GainsChanged);
	m_torso_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, TORSO_KD, (GLUI_Update_CB)GainsChanged);

	m_torso_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_torso_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Upper Left Leg");
	m_ull_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, UPPER_L_LEG_KP, (GLUI_Update_CB)GainsChanged);
	m_ull_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, UPPER_L_LEG_KD, (GLUI_Update_CB)GainsChanged);

	m_ull_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_ull_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Upper Right Leg");
	m_url_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, UPPER_R_LEG_KP, (GLUI_Update_CB)GainsChanged);
	m_url_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, UPPER_R_LEG_KD, (GLUI_Update_CB)GainsChanged);

	m_url_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_url_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Lower Left Leg");
	m_lll_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, LOWER_L_LEG_KP, (GLUI_Update_CB)GainsChanged);
	m_lll_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, LOWER_L_LEG_KD, (GLUI_Update_CB)GainsChanged);

	m_lll_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_lll_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Lower Right Leg");
	m_lrl_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, LOWER_R_LEG_KP, (GLUI_Update_CB)GainsChanged);
	m_lrl_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, LOWER_R_LEG_KD, (GLUI_Update_CB)GainsChanged);

	m_lrl_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_lrl_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Left Foot");
	m_lf_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, L_FOOT_KP, (GLUI_Update_CB)GainsChanged);
	m_lf_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, L_FOOT_KD, (GLUI_Update_CB)GainsChanged);

	m_lf_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_lf_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_separator_to_panel(gains_panel);

	m_glui_window->add_statictext_to_panel(gains_panel, "Right foot");
	m_rf_kp_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kp", GLUI_SPINNER_FLOAT, NULL, R_FOOT_KP, (GLUI_Update_CB)GainsChanged);
	m_rf_kd_spinner = m_glui_window->add_spinner_to_panel(gains_panel, "kd", GLUI_SPINNER_FLOAT, NULL, R_FOOT_KD, (GLUI_Update_CB)GainsChanged);

	m_rf_kp_spinner->set_float_limits(KP_LOWER, KP_HIGHER);
	m_rf_kd_spinner->set_float_limits(KD_LOWER, KD_HIGHER);

	m_glui_window->add_button_to_panel(gains_panel, "Save Gains", SAVEGAINS_BUTTON, (GLUI_Update_CB)SaveGainsButtonPressed);

	// Vertical separation
	m_glui_window->add_column(true);

	/*===================================== STATES =========================================*/
	GLUI_Panel *states_panel = m_glui_window->add_panel("States");
	m_StatesRadioGroup = m_glui_window->add_radiogroup_to_panel(states_panel, &m_currentState, -1, (GLUI_Update_CB)StateChanged);

	m_glui_window->add_radiobutton_to_group(m_StatesRadioGroup, "State 0");
	m_glui_window->add_radiobutton_to_group(m_StatesRadioGroup, "State 1");
	m_glui_window->add_radiobutton_to_group(m_StatesRadioGroup, "State 2");
	m_glui_window->add_radiobutton_to_group(m_StatesRadioGroup, "State 3");
	m_glui_window->add_radiobutton_to_group(m_StatesRadioGroup, "State 4");

	m_glui_window->add_separator_to_panel(states_panel);

	m_glui_window->add_statictext_to_panel(states_panel, "Desired State Angles");

	m_torso_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Torso Angle", GLUI_SPINNER_FLOAT, NULL, TORSO_ANGLE, (GLUI_Update_CB)TorsoAngleChanged);
	m_ull_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Upper left leg Angle", GLUI_SPINNER_FLOAT, NULL, ULL_ANGLE, (GLUI_Update_CB)UpperLeftLegAngleChanged);
	m_url_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Upper right leg Angle", GLUI_SPINNER_FLOAT, NULL, URL_ANGLE, (GLUI_Update_CB)UpperRightLegAngleChanged);
	m_lll_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Lower left leg Angle", GLUI_SPINNER_FLOAT, NULL, LLL_ANGLE, (GLUI_Update_CB)LowerLeftLegAngleChanged);
	m_lrl_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Lower right leg Angle", GLUI_SPINNER_FLOAT, NULL, LRL_ANGLE, (GLUI_Update_CB)LowerRightLegAngleChanged);
	m_lf_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Left foot Angle", GLUI_SPINNER_FLOAT, NULL, LF_ANGLE, (GLUI_Update_CB)LeftFootAngleChanged);
	m_rf_state_spinner = m_glui_window->add_spinner_to_panel(states_panel, "Right foot Angle", GLUI_SPINNER_FLOAT, NULL, RF_ANGLE, (GLUI_Update_CB)RightFootAngleChanged);

	m_torso_state_spinner->set_float_limits(SPINNER_TORSO_LOW, SPINNER_TORSO_HIGH);
	m_ull_state_spinner->set_float_limits(SPINNER_UPPER_LEG_LOW, SPINNER_UPPER_LEG_HIGH);
	m_url_state_spinner->set_float_limits(SPINNER_UPPER_LEG_LOW, SPINNER_UPPER_LEG_HIGH);
	m_lll_state_spinner->set_float_limits(SPINNER_LOWER_LEG_LOW, SPINNER_LOWER_LEG_HIGH);
	m_lrl_state_spinner->set_float_limits(SPINNER_LOWER_LEG_LOW, SPINNER_LOWER_LEG_HIGH);
	m_lf_state_spinner->set_float_limits(SPINNER_FOOT_LOW, SPINNER_FOOT_HIGH);
	m_rf_state_spinner->set_float_limits(SPINNER_FOOT_LOW, SPINNER_FOOT_HIGH);

	m_glui_window->add_button_to_panel(states_panel, "Save States", SAVESTATES_BUTTON, (GLUI_Update_CB)SaveStatesButtonPressed);

	/*===================================== FEEDBACK =========================================*/
	GLUI_Panel *feedback_panel = m_glui_window->add_panel("Feedback");
	m_cd_1_spinner = m_glui_window->add_spinner_to_panel(feedback_panel, "cd 1", GLUI_SPINNER_FLOAT, NULL, -1, (GLUI_Update_CB)FeedbackChanged);
	m_cv_1_spinner = m_glui_window->add_spinner_to_panel(feedback_panel, "cv 1", GLUI_SPINNER_FLOAT, NULL, -1, (GLUI_Update_CB)FeedbackChanged);
	m_cd_2_spinner = m_glui_window->add_spinner_to_panel(feedback_panel, "cd 2", GLUI_SPINNER_FLOAT, NULL, -1, (GLUI_Update_CB)FeedbackChanged);
	m_cv_2_spinner = m_glui_window->add_spinner_to_panel(feedback_panel, "cv 2", GLUI_SPINNER_FLOAT, NULL, -1, (GLUI_Update_CB)FeedbackChanged);
	m_glui_window->add_button_to_panel(feedback_panel, "Save Feedback", SAVEFEEDBACK_BUTTON, (GLUI_Update_CB)SaveFeedbackButtonPressed);

	/*===================================== STATE TIME ============================================*/
	GLUI_Panel *time_panel = m_glui_window->add_panel("State time");
	m_timer_spinner = m_glui_window->add_spinner_to_panel(time_panel, "time", GLUI_SPINNER_FLOAT, NULL, -1, (GLUI_Update_CB)TimeChanged);
	m_glui_window->add_button_to_panel(time_panel, "Save Time", SAVETIME_BUTTON, (GLUI_Update_CB)SaveTimeButtonPressed);

	/*===================================== CONTROLS =========================================*/

	GLUI_Panel *control_panel = m_glui_window->add_panel("Controls");
	m_glui_window->add_button_to_panel(control_panel, "Reset", RESET_BUTTON, (GLUI_Update_CB)ResetButtonPressed);
	m_glui_window->add_separator_to_panel(control_panel);
	m_glui_window->add_button_to_panel(control_panel, "Pause", PAUSE_BUTTON, (GLUI_Update_CB)PauseButtonPressed);
	m_glui_window->add_separator_to_panel(control_panel);
	m_glui_window->add_button_to_panel(control_panel, "Start", START_BUTTON, (GLUI_Update_CB)StartButtonPressed);

	//// Vertical separation
	m_glui_window->add_column(true);

	GLUI_Panel *gait_panel = m_glui_window->add_panel("Gaits");
	m_GaitsRadioGroup = m_glui_window->add_radiogroup_to_panel(gait_panel, &m_currentGait, -1, (GLUI_Update_CB)GaitsChanged);
	// Determine how many gaits we have.
	int walk_index = 0;
	int ind = 0;
	for (std::vector<std::string>::iterator it = m_gaits.begin(); it != m_gaits.end(); ++it) {
		std::string gait_name = *it;
		m_glui_window->add_radiobutton_to_group(m_GaitsRadioGroup, gait_name.c_str());
		if (strcmp(gait_name.c_str(), "Walk") == 0) {
			walk_index = ind;
		}
		ind++;
	}
	m_GaitsRadioGroup->set_int_val(walk_index);
	m_previousGait = walk_index;
	m_currentGait = walk_index;

}

void RagDoll::SetupGUIConfiguration() {

	// Assume Currently Selected State is 0
	DisplayGains();

	DisplayFeedback();
	DisplayTime();

	DisableStateSpinner();

}

void RagDoll::DisplayState(int state) {

	State *selected_state = NULL;
	selected_state = GetState(state);

	m_torso_state_spinner->set_float_val(selected_state->m_torsoAngle);

	m_ull_state_spinner->set_float_val(selected_state->m_upperLeftLegAngle);
	m_url_state_spinner->set_float_val(selected_state->m_upperRightLegAngle);

	// Lower legs have to relative orientation to upper legs. Local coordinates
	m_lll_state_spinner->set_float_val(selected_state->m_lowerLeftLegAngle);
	m_lrl_state_spinner->set_float_val(selected_state->m_lowerRightLegAngle);

	m_lf_state_spinner->set_float_val(selected_state->m_leftFootAngle);
	m_rf_state_spinner->set_float_val(selected_state->m_rightFootAngle);

}

void RagDoll::DisplayGains() {

	m_torso_kp_spinner->set_float_val(m_WalkingController->m_torso_gains->m_kp);
	m_torso_kd_spinner->set_float_val(m_WalkingController->m_torso_gains->m_kd);

	m_ull_kp_spinner->set_float_val(m_WalkingController->m_ull_gains->m_kp);
	m_ull_kd_spinner->set_float_val(m_WalkingController->m_ull_gains->m_kd);

	m_url_kp_spinner->set_float_val(m_WalkingController->m_url_gains->m_kp);
	m_url_kd_spinner->set_float_val(m_WalkingController->m_url_gains->m_kd);

	m_lll_kp_spinner->set_float_val(m_WalkingController->m_lll_gains->m_kp);
	m_lll_kd_spinner->set_float_val(m_WalkingController->m_lll_gains->m_kd);

	m_lrl_kp_spinner->set_float_val(m_WalkingController->m_lrl_gains->m_kp);
	m_lrl_kd_spinner->set_float_val(m_WalkingController->m_lrl_gains->m_kd);

	m_lf_kp_spinner->set_float_val(m_WalkingController->m_lf_gains->m_kp);
	m_lf_kd_spinner->set_float_val(m_WalkingController->m_lf_gains->m_kd);

	m_rf_kp_spinner->set_float_val(m_WalkingController->m_rf_gains->m_kp);
	m_rf_kd_spinner->set_float_val(m_WalkingController->m_rf_gains->m_kd);


}

void RagDoll::DisplayFeedback() {

	m_cd_1_spinner->set_float_val(*m_WalkingController->m_cd_1);
	m_cv_1_spinner->set_float_val(*m_WalkingController->m_cv_1);
	m_cd_2_spinner->set_float_val(*m_WalkingController->m_cd_2);
	m_cv_2_spinner->set_float_val(*m_WalkingController->m_cv_2);

}

void RagDoll::DisplayTime() {

	m_timer_spinner->set_float_val(m_WalkingController->m_state_time);

}

void RagDoll::UpdateGains() {
	m_WalkingController->SetTorsoGains(m_torso_kp_spinner->get_float_val(), m_torso_kd_spinner->get_float_val());
	m_WalkingController->SetUpperLeftLegGains(m_ull_kp_spinner->get_float_val(), m_ull_kd_spinner->get_float_val());
	m_WalkingController->SetUpperRightLegGains(m_url_kp_spinner->get_float_val(), m_url_kd_spinner->get_float_val());
	m_WalkingController->SetLowerLeftLegGains(m_lll_kp_spinner->get_float_val(), m_lll_kd_spinner->get_float_val());
	m_WalkingController->SetLowerRightLegGains(m_lrl_kp_spinner->get_float_val(), m_lrl_kd_spinner->get_float_val());
	m_WalkingController->SetLeftFootGains(m_lf_kp_spinner->get_float_val(), m_lf_kd_spinner->get_float_val());
	m_WalkingController->SetRightFootGains(m_rf_kp_spinner->get_float_val(), m_rf_kd_spinner->get_float_val());
}

void RagDoll::UpdateFeedbacks() {

	m_WalkingController->SetFeedback1(m_cd_1_spinner->get_float_val(), m_cv_1_spinner->get_float_val());
	m_WalkingController->SetFeedback2(m_cd_2_spinner->get_float_val(), m_cv_2_spinner->get_float_val());
}

void RagDoll::DisableStateSpinner() {

	switch (m_currentState)
	{
	case 0: {
		// Deactivate
		m_url_state_spinner->disable();
		m_lrl_state_spinner->disable();
		m_rf_state_spinner->disable();
		m_ull_state_spinner->disable();
		m_lll_state_spinner->disable();
		m_lf_state_spinner->disable();
		m_torso_state_spinner->disable();
	}
			break;
	case 1:
	case 2: {
		m_url_state_spinner->enable();
		m_lrl_state_spinner->enable();
		m_rf_state_spinner->enable();
		m_torso_state_spinner->enable();
		m_ull_state_spinner->enable();
		m_lll_state_spinner->enable();
		m_lf_state_spinner->enable();
		break;
	}
	case 3:
	case 4:{
		m_url_state_spinner->enable();
		m_lrl_state_spinner->enable();
		m_rf_state_spinner->enable();
		m_torso_state_spinner->enable();
		m_ull_state_spinner->enable();
		m_lll_state_spinner->enable();
		m_lf_state_spinner->enable();
	}
		   break;
	default:
		break;
	}

}

void RagDoll::DisableAllSpinners() {

	std::vector<GLUI_Spinner*>spinners = {
		m_torso_kp_spinner, m_torso_kd_spinner,
		m_ull_kp_spinner, m_ull_kd_spinner,
		m_url_kp_spinner, m_url_kd_spinner,
		m_lll_kp_spinner, m_lll_kd_spinner,
		m_lrl_kp_spinner, m_lrl_kd_spinner,
		m_lf_kp_spinner, m_lf_kd_spinner,
		m_rf_kp_spinner, m_rf_kd_spinner,
		m_torso_state_spinner, m_ull_state_spinner,
		m_url_state_spinner, m_lll_state_spinner,
		m_lrl_state_spinner, m_lf_state_spinner,
		m_rf_state_spinner
	};

	for (std::vector<GLUI_Spinner*>::iterator it = spinners.begin(); it != spinners.end(); it++) {
		(*it)->disable();
	}

	m_StatesRadioGroup->disable();

}

void RagDoll::EnableGainSpinners() {

	std::vector<GLUI_Spinner*>spinners = {
		m_torso_kp_spinner, m_torso_kd_spinner,
		m_ull_kp_spinner, m_ull_kd_spinner,
		m_url_kp_spinner, m_url_kd_spinner,
		m_lll_kp_spinner, m_lll_kd_spinner,
		m_lrl_kp_spinner, m_lrl_kd_spinner,
		m_lf_kp_spinner, m_lf_kd_spinner,
		m_rf_kp_spinner, m_rf_kd_spinner
	};

	for (std::vector<GLUI_Spinner*>::iterator it = spinners.begin(); it != spinners.end(); it++) {
		(*it)->enable();
	}

}

void RagDoll::SaveStates() {
	//ChangeState(-1);
	// Save States into file
	m_WalkingController->SaveStates(m_gaits.at(m_currentGait));
}

void RagDoll::SaveGains(){
	// Save gains into file
	m_WalkingController->SaveGains(m_gaits.at(m_currentGait));
}

void RagDoll::SaveFeedback() {
	m_WalkingController->SaveFeedback(m_gaits.at(m_currentGait));
}

void RagDoll::SaveTime() {
	m_WalkingController->SaveTime(m_gaits.at(m_currentGait));
}

void RagDoll::Reset() {

	Debug("Reset button pressed");

	m_WalkingController->Reset();

	// Clear Everything
	GameObject::ClearForces(m_bodies);
	GameObject::ClearVelocities(m_bodies);

	m_StatesRadioGroup->enable();

	GameObject::DisableObjects(m_bodies);
	
	m_StatesRadioGroup->set_int_val(0); // State 0
	m_currentState = 0;
	ChangeState(0);
	EnableGainSpinners();

	try {
		m_ResetCallback();
	}
	catch (const std::bad_function_call& e) {
		//std::cout << e.what() << '\n';
	}

	m_disabled = true;

}

void RagDoll::Start() {

	try {
		m_StartCallback();
	}
	catch (const std::bad_function_call& e) {
		//std::cout << e.what() << '\n';
	}

	DisableAllSpinners();
	// Clear Everything
	Debug("Start button Pressed\n INITIATE WALKING!!!");
	GameObject::EnableObjects(m_bodies);
	
	m_disabled = false;

	m_WalkingController->InitiateWalking();

	//GameObject::EnableObjects({ randomBox});
	//GameObject::EnableObjects({ randomBox, randomBox2 });

}

void RagDoll::Pause() {

	Debug("Pause button pressed");

	GameObject::DisableObjects(m_bodies);

	m_WalkingController->PauseWalking();
}

void RagDoll::ChangeState(int id) {

	//printf("Gait is %s \n", m_gaits.at(m_currentGait).c_str());

	Debug("Previous state" << m_previousState);

	State *previousState = NULL;

	previousState = GetState(m_previousState);

	// Update previous state
	previousState->m_torsoAngle = m_torso_state_spinner->get_float_val();
	previousState->m_upperLeftLegAngle = m_ull_state_spinner->get_float_val();
	previousState->m_upperRightLegAngle = m_url_state_spinner->get_float_val();
	previousState->m_lowerLeftLegAngle = m_lll_state_spinner->get_float_val();
	previousState->m_lowerRightLegAngle = m_lrl_state_spinner->get_float_val();
	previousState->m_leftFootAngle = m_lf_state_spinner->get_float_val();
	previousState->m_rightFootAngle = m_rf_state_spinner->get_float_val();

	// Change previous to current state
	DisplayState(m_currentState);
	m_previousState = m_currentState;
	Debug("next state = " << m_currentState);

	switch (m_currentState)
	{
	case 0:
		m_WalkingController->m_ragDollState = STATE_0;
		break;
	case 1:
		m_WalkingController->m_ragDollState = STATE_1;
		break;
	case 2:
		m_WalkingController->m_ragDollState = STATE_2;
		break;
	case 3:
		m_WalkingController->m_ragDollState = STATE_3;
		break;
	case 4:
		m_WalkingController->m_ragDollState = STATE_4;
		break;
	default:
		break;
	}

	DisableStateSpinner();

	UpdateRagDoll();

}

void RagDoll::ChangeGait() {

	printf("Previous gate: %s, Current Gait: %s \n", m_gaits.at(m_previousGait).c_str(), m_gaits.at(m_currentGait).c_str());

	m_previousGait = m_currentGait;
	m_WalkingController->ChangeGait(m_gaits.at(m_currentGait));
	SetupGUIConfiguration();
	DisplayState(m_currentState);

	if (m_WalkingController->m_currentState == RESET) {
		UpdateRagDoll();
	}

}

void RagDoll::UpdateTime() {
	m_WalkingController->SetStateTime(m_timer_spinner->get_float_val());
	SetupGUIConfiguration();
}

void RagDoll::ChangeTorsoAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_torsoAngle = m_torso_state_spinner->get_float_val();
	UpdateRagDoll();
}

void RagDoll::ChangeUpperLeftLegAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_upperLeftLegAngle = m_ull_state_spinner->get_float_val();
	UpdateRagDoll();
}

void RagDoll::ChangeUpperRightLegAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_upperRightLegAngle = m_url_state_spinner->get_float_val();
	UpdateRagDoll();
}

void RagDoll::ChangeLowerLeftLegAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_lowerLeftLegAngle = m_lll_state_spinner->get_float_val(); // Assume relative orientation
	UpdateRagDoll();
}

void RagDoll::ChangeLowerRightLegAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_lowerRightLegAngle = m_lrl_state_spinner->get_float_val(); // Assume relative orientation
	UpdateRagDoll();
}

void RagDoll::ChangeLeftFootAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_leftFootAngle = m_lf_state_spinner->get_float_val(); // Assume relative orientation
	UpdateRagDoll();
}

void RagDoll::ChangeRightFootAngle() {
	State *state = NULL;
	state = GetState(m_currentState);
	state->m_rightFootAngle = m_rf_state_spinner->get_float_val(); // Assume relative orientation
	UpdateRagDoll();
}

#pragma endregion GUI

#pragma region TORQUES

// Upper legs
void RagDoll::ApplyTorqueOnUpperLeftLeg(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_upperLeftLeg, torque);
}

void RagDoll::ApplyTorqueOnUpperRightLeg(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_upperRightLeg, torque);
}

// Lower legs
void RagDoll::ApplyTorqueOnLowerLeftLeg(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_lowerLeftLeg, torque);
}

void RagDoll::ApplyTorqueOnLowerRightLeg(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_lowerRightLeg, torque);
}

// Feet
void RagDoll::ApplyTorqueOnLeftFoot(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_leftFoot, torque);
}

void RagDoll::ApplyTorqueOnRightFoot(float torqueForce) {
	btVector3 torque(btVector3(0, 0, torqueForce));
	m_app->ApplyTorque(m_rightFoot, torque);
}

#pragma endregion TORQUES

#pragma region DRAWING

bool RagDoll::DrawShape(btScalar *transform, const btCollisionShape *pShape, const btVector3 &color) {

	bool takenCareOf = true;

	// Special rendering
	if (pShape->getUserPointer() == m_torso) {
		const btBoxShape *box = static_cast<const btBoxShape*>(pShape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();

		glColor3f(color.x(), color.y(), color.z());

		// push the matrix stack

		glPushMatrix();
		glMultMatrixf(transform);

		DrawTorso(halfSize);

		glPopMatrix();
	}
	else if (pShape->getUserPointer() == m_upperLeftLeg || pShape->getUserPointer() == m_upperRightLeg) {
		// Draw Upper legs
		const btBoxShape *box = static_cast<const btBoxShape*>(pShape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		glColor3f(color.x(), color.y(), color.z());
		glPushMatrix();
		glMultMatrixf(transform);

		DrawUpperLeg(halfSize);

		glPopMatrix();

	}
	else if (pShape->getUserPointer() == m_lowerLeftLeg || pShape->getUserPointer() == m_lowerRightLeg) {
		// Draw Lower legs
		const btBoxShape *box = static_cast<const btBoxShape*>(pShape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		glColor3f(color.x(), color.y(), color.z());
		glPushMatrix();
		glMultMatrixf(transform);

		DrawLowerLeg(halfSize);

		glPopMatrix();

	}
	else if (pShape->getUserPointer() == m_leftFoot || pShape->getUserPointer() == m_rightFoot) {
		// Draw feet
		const btBoxShape *box = static_cast<const btBoxShape*>(pShape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();

		glColor3f(color.x(), color.y(), color.z());
		glPushMatrix();
		glMultMatrixf(transform);

		DrawFoot(halfSize);

		glPopMatrix();

	}
	else {
		takenCareOf = false;
	}

	return takenCareOf;

}

void RagDoll::DrawTorso(const btVector3 &halfSize) {

	float halfHeight = halfSize.y();
	float halfWidth = halfSize.x();
	float halfDepth = halfSize.z(); // No depth

	float shoulderRadius = 1.5 * halfWidth;
	float hipRadius = 1.25 * halfWidth;

	// Create Vector
	btVector3 vertices[4] = {
		btVector3(-shoulderRadius, halfHeight, 0.0f),	// 0
		btVector3(shoulderRadius, halfHeight, 0.0f),	// 1
		btVector3(-hipRadius, -halfHeight, 0.0f),		// 2 
		btVector3(hipRadius, -halfHeight, 0.0f),		// 3

	};

	static int indices[6] = {
		0, 1, 2,
		3, 2, 1
	};

	DrawWithTriangles(vertices, indices, 6);

	// Create semisircle for shoulders
	DrawPartialFilledCircle(0, halfHeight, shoulderRadius, 0, 180);
	DrawPartialFilledCircle(0, -2 * halfHeight, hipRadius, 180, 360);

}

void RagDoll::DrawUpperLeg(const btVector3 &halfSize) {

	float halfHeight = halfSize.y();
	float halfWidth = halfSize.x();
	float halfDepth = halfSize.z(); // No depth

	float thighRadius = 1.25f * halfWidth;
	float kneeRadius = .9f * halfWidth;

	// Create Vector
	btVector3 vertices[4] = {
		btVector3(-thighRadius, halfHeight, 0.0f),		// 0
		btVector3(thighRadius, halfHeight, 0.0f),		// 1
		btVector3(-kneeRadius, -halfHeight, 0.0f),		// 2 
		btVector3(kneeRadius, -halfHeight, 0.0f),		// 3

	};

	static int indices[6] = {
		0, 1, 2,
		3, 2, 1
	};

	DrawWithTriangles(vertices, indices, 6);

	// Create semisircle for thigh and knees
	DrawPartialFilledCircle(0, halfHeight, thighRadius, 0, 180);
	DrawPartialFilledCircle(0, -2 * halfHeight, kneeRadius, 180, 360);

}

void RagDoll::DrawLowerLeg(const btVector3 &halfSize){

	float halfHeight = halfSize.y();
	float halfWidth = halfSize.x();
	float halfDepth = halfSize.z(); // No depth

	float kneeRadius = 1.25f * halfWidth;
	float ankleRadius = halfWidth;

	// Create Vector
	btVector3 vertices[4] = {
		btVector3(-kneeRadius, halfHeight, 0.0f),		// 0
		btVector3(kneeRadius, halfHeight, 0.0f),		// 1
		btVector3(-ankleRadius, -halfHeight, 0.0f),	// 2 
		btVector3(ankleRadius, -halfHeight, 0.0f),		// 3

	};

	static int indices[6] = {
		0, 1, 2,
		3, 2, 1
	};

	DrawWithTriangles(vertices, indices, 6);

	// Create semisircle for thigh and knees
	DrawPartialFilledCircle(0, halfHeight, kneeRadius, 0, 180);
	//DrawPartialFilledCircle(-2*halfHeight, 0, ankleRadius, 90, 270);

}

void RagDoll::DrawFoot(const btVector3 &halfSize) {

	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z(); // No depth

	float toeRadius = halfHeight * 2;

	// Create Vector
	btVector3 vertices[4] = {
		btVector3(-halfWidth, halfHeight, 0.0f),		// 0
		btVector3(-halfWidth, -halfHeight, 0.0f),	// 1
		btVector3((halfWidth - 2 * halfHeight), -halfHeight, 0.0f),	// 2 
		btVector3((halfWidth - 2 * halfHeight), halfHeight, 0.0f),	// 3

	};

	static int indices[6] = {
		0, 1, 2,
		3, 2, 0
	};

	DrawWithTriangles(vertices, indices, 6);

	// Create quarter circle for toes
	//DrawPartialFilledCircle(, , toeRadius, 0, 180);
	DrawPartialFilledCircle((halfWidth - 2 * halfHeight), -halfHeight, toeRadius, 0, 90);

}

static void DrawPartialFilledCircle(GLfloat x, GLfloat y, GLfloat radius, GLfloat begin, GLfloat end) {
	/* Draw CCW from begin to end */
	int triangleAmount = 20; //# of triangles used to draw circle

	glTranslatef(x, y, 0);

	begin = Constants::GetInstance().DegreesToRadians(begin);
	end = Constants::GetInstance().DegreesToRadians(end);

	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(0, 0);
	for (int i = 0; i <= triangleAmount; i++) {
		glVertex2f(
			(radius * cos(i *  (end - begin) / triangleAmount + begin)),
			(radius * sin(i * (end - begin) / triangleAmount + begin))
			);
	}
	glEnd();

}

static void DrawFilledCircle(GLfloat x, GLfloat y, GLfloat radius, const btVector3 &color){
	int triangleAmount = 20; //# of triangles used to draw circle
	glColor3f(color.x(), color.y(), color.z());
	glPushMatrix();
	glTranslatef(x, y, 0.9);
	//GLfloat radius = 0.8f; //radius

	glBegin(GL_TRIANGLE_FAN);
	for (int i = 0; i <= triangleAmount; i++) {
		glVertex2f(
			(radius * cos(i *  TWO_PI / triangleAmount)),
			(radius * sin(i * TWO_PI / triangleAmount))
			);
	}
	glEnd();

	glPopMatrix();
}

#pragma endregion DRAWING

#pragma region GLUI_CALLBACKS

static void RagDollIdle() {
	m_ragDoll->Idle();
}

static void SaveGainsButtonPressed(int id) {
	m_ragDoll->SaveGains();
}

static void SaveStatesButtonPressed(int id) {
	m_ragDoll->SaveStates();
}

static void SaveFeedbackButtonPressed(int id) {
	m_ragDoll->SaveFeedback();
}

static void SaveTimeButtonPressed(int id) {
	m_ragDoll->SaveTime();
}

static void ResetButtonPressed(int id) {
	m_ragDoll->Reset();
}

static void StartButtonPressed(int id) {
	m_ragDoll->Start();
}

static void PauseButtonPressed(int id) {
	m_ragDoll->Pause();
}

static void StateChanged(int id)	 {
	m_ragDoll->ChangeState(id);
}

static void GaitsChanged(int id) {
	m_ragDoll->ChangeGait();
}

// State Callbacks

static void TorsoAngleChanged(int id) {
	m_ragDoll->ChangeTorsoAngle();
}

static void UpperLeftLegAngleChanged(int id) {
	m_ragDoll->ChangeUpperLeftLegAngle();
}

static void UpperRightLegAngleChanged(int id) {
	m_ragDoll->ChangeUpperRightLegAngle();
}

static void LowerLeftLegAngleChanged(int id) {
	m_ragDoll->ChangeLowerLeftLegAngle();
}

static void LowerRightLegAngleChanged(int id) {
	m_ragDoll->ChangeLowerRightLegAngle();
}

static void LeftFootAngleChanged(int id) {
	m_ragDoll->ChangeLeftFootAngle();
}

static void RightFootAngleChanged(int id) {
	m_ragDoll->ChangeRightFootAngle();
}

static void GainsChanged(int id) {
	m_ragDoll->UpdateGains();
}

static void FeedbackChanged(int id) {
	m_ragDoll->UpdateFeedbacks();
}

static void TimeChanged(int id) {
	m_ragDoll->UpdateTime();
}

#pragma endregion GLUI_CALLBACKS
