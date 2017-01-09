#pragma once
#include "BulletOpenGLApplication.h"
#include <deque>
#include <random>
#include <chrono>
#include <memory>
#include <stdint.h>
#include <cassert>
#include <iostream>

#include "ContactCollisionDispatcher.h"
#include "RagDoll.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include "SQL_DataWrapper.h"

class GLUI;
class TactileController;
class ColliderObject;
class TerrainCreator;

#define GROUND_WIDTH 2
#define GROUND_HEIGHT 0.25f
#define GROUND_DEPTH 1.0f
#define ORIG_POS_FEELER btVector3(5, 2, 0)
#define RESET_TIME 1.0f

enum BodyTag {GROUND = 0};

class ContactLearningApp :
	public BulletOpenGLApplication
{
public:
	ContactLearningApp();
	~ContactLearningApp();

	ContactLearningApp(ProjectionMode mode);

	static ContactLearningApp* GetInstance();

	virtual void InitializePhysics() override;
	virtual void ShutdownPhysics() override;

	void LoadTextures();
	void CreateGround(const btVector3 &pos = btVector3(0,0,0));
	void CreateBodies();

	void DrawShapeCallback(btScalar *transform, const btCollisionShape *shape, const btVector3 &color);
	void DrawCallback();

	virtual void Idle() override;

	void PostTickCallback(btScalar timestep);
	void PreTickCallback(btScalar timestep);

	void ManageTerrainCreation();
	void CreateMoarTerrain();
	void ManageGroundCollisions();

	GameObject *GetCurrentStandingGround();

	void Start();
	void Reset();
	void SimulationEnd();

	/* Data base stuff*/
	void InitializeDB();
	void SaveSamplesToDB(SQL_DataWrapper data);
	void SaveSequenceToDB(int sequenceNumber, int rowId, int order);
	
	std::mutex m_mutex;
	std::deque<SQL_DataWrapper> m_data;
	std::condition_variable m_NotEmptyCV;
	//std::condition_variable m_ProcessedCV;
	bool m_Ready = false;
	bool m_Processed = false;
	std::thread m_dbSaver;
	void DBWorkerThread();
	SQL_DataWrapper PushDataIntoQueue();

	int m_sequenceNumber = -1;
	int m_newestID = -1;
	int m_order = -1;

	bool m_running = false;

private:

	GLUI *m_glui_window;
	GameObject *m_block;
	GLuint m_ground_texture;

	ContactCollisionDispatcher *m_pCollisionDispatcher;

	btVector3 m_groundEndPt;
	btVector3 m_collisionEndPt;

	std::unique_ptr<TerrainCreator> tc;

	std::unique_ptr<RagDoll> m_ragDoll;

	std::deque<GameObject *> m_grounds;
	int m_ground_idx;
	std::deque<GameObject *> m_collisionGrounds;

	btClock m_collisionClock;
	btClock m_sampleClock;
	btClock m_generationClock;
	btClock m_resetTimerClock;

	bool m_restart;

	// Database
	sqlite3 *m_samplesdb;

};

void InternalPostTickCallback(btDynamicsWorld *world, btScalar timestep);
void InternalPreTickCallback(btDynamicsWorld *world, btScalar timestep);

/* SQL Callbacks */
static int NumSequencesCallback(void *data, int argc, char **argv, char **azColName);

const char *m_insert_states_sql_cmd = "INSERT INTO STATES(" \
"ID, " \
"TORSO_LV_X, " \
"TORSO_LV_Y, " \
"TORSO_D, " \
"TORSO_O, " \
"TORSO_AV, " \
"URL_D, " \
"URL_O," \
"URL_AV, " \
"ULL_D, " \
"ULL_O, " \
"ULL_AV, " \
"LRL_D, " \
"LRL_O, " \
"LRL_AV, " \
"LLL_D, " \
"LLL_O, " \
"LLL_AV, " \
"RF_D, " \
"RF_O, " \
"RF_AV, " \
"LF_D, " \
"LF_O, " \
"LF_AV, " \
"RF_FORCES, " \
"LF_FORCES, " \
"GROUND_STIFFNESS, " \
"GROUND_DAMPING, " \
"GROUND_SLOPE)" \
"VALUES(NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,? , ?, ?, ?, ?, ? )";

const char *m_insert_sequences_cmd = "INSERT INTO SEQUENCES(" \
"SEQUENCE_ID, " \
"STATE_ID, " \
"SEQ_ORDER) " \
"VALUES(?, ?, ?)";