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

class GLUI;
class TactileController;
class ColliderObject;
class TerrainCreator;

#define GROUND_WIDTH 3
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

	void Start();
	void Reset();
	void SimulationEnd();

	/* Data base stuff*/
	void InitializeDB();
	void SaveSamplesToDB();
	void SaveSequenceToDB(int rowId);
	
	std::mutex m_mutex;
	//std::unique_lock <std::mutex> m_lk;
	std::condition_variable m_ReadyCV;
	std::condition_variable m_ProcessedCV;
	bool m_Ready = false;
	bool m_Processed = false;
	std::thread m_dbSaver;
	void DBWorkerThread();

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

	// Database
	sqlite3 *m_samplesdb;

};

void InternalPostTickCallback(btDynamicsWorld *world, btScalar timestep);
void InternalPreTickCallback(btDynamicsWorld *world, btScalar timestep);

/* SQL Callbacks */
static int NumSequencesCallback(void *data, int argc, char **argv, char **azColName);