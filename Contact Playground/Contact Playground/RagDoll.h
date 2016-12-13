#pragma once

#include "GameObject.h"
#include <glui\glui.h>
#include <deque>

#include <functional>
#include <memory>

class State;
class Gains;
class WalkingController;
class BulletOpenGLApplication;
class ColliderObject;

class sqlite3;

class RagDoll
{
public:
	RagDoll(BulletOpenGLApplication *app, bool useCustomContact = false);
	~RagDoll();

	void InitializeRagDoll(float halfGroundHeight, int window_id);

	void CreateRagDoll(const btVector3 &position);
	void AddHinges();

	void ConfigureContactModel();

	GameObject *Create2DBox(const btVector3 &halfSize, float mass, const btVector3 &color, const btVector3 &position, std::string name="Object");

	void Loop();
	void AddCollisionWithGround(GameObject *ground);
	void RemoveCollisionWithGround(GameObject *ground);

	// Rag Doll model
	GameObject *m_torso;
	GameObject *m_upperRightLeg;
	GameObject *m_upperLeftLeg;
	GameObject *m_lowerRightLeg;
	GameObject *m_lowerLeftLeg;
	GameObject *m_rightFoot;
	GameObject *m_leftFoot;

	//GameObject *randomBox;

	// Custom contact
	std::unique_ptr<ColliderObject> m_leftFootCollider;
	std::unique_ptr<ColliderObject> m_rightFootCollider;
	std::unique_ptr<ColliderObject> m_torsoCollider;

	// GUI 
	void Reset();
	void SaveStates();
	void SaveGains();
	void SaveFeedback();
	void SaveTime();
	void Start();
	void Pause();
	void CloseGLUIWindow(int id);
	void ChangeState(int id);
	void ChangeGait();
	void ChangeTorsoAngle();
	void ChangeUpperLeftLegAngle();
	void ChangeUpperRightLegAngle();
	void ChangeLowerLeftLegAngle();
	void ChangeLowerRightLegAngle();
	void ChangeLeftFootAngle();
	void ChangeRightFootAngle();
	void UpdateGains();
	void UpdateFeedbacks();
	void UpdateTime();
	void SetupGUIConfiguration();

	// Forces
	void ApplyTorqueOnUpperRightLeg(float torqueForce);
	void ApplyTorqueOnUpperLeftLeg(float torqueForce);
	void ApplyTorqueOnLowerRightLeg(float torqueForce);
	void ApplyTorqueOnLowerLeftLeg(float torqueForce);
	void ApplyTorqueOnRightFoot(float torqueForce);
	void ApplyTorqueOnLeftFoot(float torqueForce);

	// Physics step
	void RagDollStep(btScalar timestep);
	void RagDollCollision(btScalar timestep, btDynamicsWorld *world, std::deque<GameObject *>& grounds);

	State *GetState(int state);

	void Idle();

	// Location of Rag Doll
	btVector3 GetLocation();

	// Drawing
	bool DrawShape(btScalar *transform, const btCollisionShape *pShape, const btVector3 &color);

	std::function<void()> m_ResetCallback;
	std::function<void()> m_StartCallback;
	std::function<void()> m_SimulationEnd;
	
	bool m_disabled = true;

	// Get Data
	std::vector<float> GetOrientationsAndAngularVelocities();
	btVector3 GetTorsoLinearVelocity();
	std::pair<std::vector<btVector3>, std::vector<btVector3>> GetContactForces();

private:

	void DisplayState(int state);
	void DisplayGains();
	void DisplayFeedback();
	void DisplayTime();
	void DisableStateSpinner();
	void DisableAllSpinners();
	void EnableGainSpinners();
	void UpdateRagDoll();

	// Drawing
	void DrawTorso(const btVector3 &halfSize);
	void DrawUpperLeg(const btVector3 &halfSize);
	void DrawLowerLeg(const btVector3 &halfSize);
	void DrawFoot(const btVector3 &halfSize);


	WalkingController *m_WalkingController;

	btClock m_collisionClock;

	btVector3 m_origTorsoPosition;

	// GLUI
	GLUI *m_glui_window;
	int m_currentState = 0;
	int m_previousState = 0;

	int m_currentGait;
	int m_previousGait;

	bool m_useCustomContact;

	std::vector<GameObject *> m_bodies;

	btHingeConstraint *m_torso_urLeg;
	btHingeConstraint *m_torso_ulLeg;
	btHingeConstraint *m_urLeg_lrLeg;
	btHingeConstraint *m_ulLeg_llLeg;
	btHingeConstraint *m_lrLeg_rFoot;
	btHingeConstraint *m_llLeg_lFoot;

	// Gaits
	std::vector<std::string> m_gaits;

	void CreateRagDollGUI(int window_id);

	// GLUI Members

	GLUI_Spinner *m_torso_kp_spinner;
	GLUI_Spinner *m_torso_kd_spinner;

	GLUI_Spinner *m_ull_kp_spinner;
	GLUI_Spinner *m_ull_kd_spinner;

	GLUI_Spinner *m_url_kp_spinner;
	GLUI_Spinner *m_url_kd_spinner;

	GLUI_Spinner *m_lll_kp_spinner;
	GLUI_Spinner *m_lll_kd_spinner;

	GLUI_Spinner *m_lrl_kp_spinner;
	GLUI_Spinner *m_lrl_kd_spinner;

	GLUI_Spinner *m_lf_kp_spinner;
	GLUI_Spinner *m_lf_kd_spinner;

	GLUI_Spinner *m_rf_kp_spinner;
	GLUI_Spinner *m_rf_kd_spinner;

	GLUI_RadioGroup *m_StatesRadioGroup;
	GLUI_Spinner *m_torso_state_spinner;
	GLUI_Spinner *m_ull_state_spinner;
	GLUI_Spinner *m_url_state_spinner;
	GLUI_Spinner *m_lll_state_spinner;
	GLUI_Spinner *m_lrl_state_spinner;
	GLUI_Spinner *m_lf_state_spinner;
	GLUI_Spinner *m_rf_state_spinner;

	GLUI_Spinner *m_cd_1_spinner;
	GLUI_Spinner *m_cv_1_spinner;
	GLUI_Spinner *m_cd_2_spinner;
	GLUI_Spinner *m_cv_2_spinner;

	GLUI_Spinner *m_timer_spinner;

	GLUI_RadioGroup *m_GaitsRadioGroup;

	BulletOpenGLApplication *m_app;
	
};

/* GLUI CALLBACKS */
static void RagDollIdle();
static void SaveGainsButtonPressed(int id);
static void SaveStatesButtonPressed(int id);
static void SaveFeedbackButtonPressed(int id);
static void SaveTimeButtonPressed(int id);
static void ResetButtonPressed(int id);
static void PauseButtonPressed(int id);
static void StartButtonPressed(int id);
static void StateChanged(int id);
static void GaitsChanged(int id);

static void TorsoAngleChanged(int id);
static void UpperLeftLegAngleChanged(int id);
static void UpperRightLegAngleChanged(int id);
static void LowerLeftLegAngleChanged(int id);
static void LowerRightLegAngleChanged(int id);
static void LeftFootAngleChanged(int id);
static void RightFootAngleChanged(int id);
static void GainsChanged(int id);
static void FeedbackChanged(int id);
static void TimeChanged(int id);

static void DrawFilledCircle(GLfloat x, GLfloat y, GLfloat radius, const btVector3 &color = btVector3(255, 255, 255));
static void DrawPartialFilledCircle(GLfloat x, GLfloat y, GLfloat radius, GLfloat begin, GLfloat end); // Draw CCW from begin to end
static void DrawLine(const btVector3 &begin, const btVector3 &end, const btVector3 &color);