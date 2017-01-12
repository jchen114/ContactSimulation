#include "stdafx.h"
#include <boost\asio.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>
#include "ContactLearningApp.h"
#include <SOIL\src\SOIL.h>
#include "ContactManager.h"
#include <glui\glui.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <ctime>
#include <stdexcept>

#include "TerrainCreator.h"

//std::string db_path = "..\\..\\Data\\samples.db";
std::string db_path = "..\\..\\Data\\samples_w_compliance.db";

#include "sqlite3.h"

using namespace std::placeholders;

static ContactLearningApp *m_app;

#pragma region INIT

ContactLearningApp::ContactLearningApp()
{
}

ContactLearningApp::~ContactLearningApp()
{
}

ContactLearningApp::ContactLearningApp(ProjectionMode mode) : BulletOpenGLApplication(mode, false, btVector3(0, 0, 0), 4.0f, 20.0f, 0.0f)
{
	m_app = this;
	m_DrawShapeCallback = std::bind(&ContactLearningApp::DrawShapeCallback, this, _1, _2, _3);
	m_DrawCallback = std::bind(&ContactLearningApp::DrawCallback, this);

	tc = std::unique_ptr<TerrainCreator>(new TerrainCreator(GROUND_WIDTH, 0.001f, GROUND_HEIGHT, GROUND_DEPTH));

	srand(time(NULL));
}

ContactLearningApp *ContactLearningApp::GetInstance() {
	return m_app;
}

void ContactLearningApp::ShutdownPhysics() {

	delete m_pWorld;
	delete m_pSolver;
	delete m_pBroadphase;
	delete m_pDispatcher;
	delete m_pCollisionDispatcher;
	delete m_pCollisionConfiguration;

	delete m_glui_window;

}

void ContactLearningApp::InitializePhysics() {

	// create the collision configuration
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
	// create the dispatcher
	m_pDispatcher = new btCollisionDispatcher(m_pCollisionConfiguration);
	//m_pCollisionDispatcher = new ContactCollisionDispatcher(m_pCollisionConfiguration);
	// Adding for 2D collisions and solving
	m_pDispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, new btBox2dBox2dCollisionAlgorithm::CreateFunc());
	//m_pCollisionDispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, new btBox2dBox2dCollisionAlgorithm::CreateFunc());
	
	// create the broadphase
	m_pBroadphase = new btDbvtBroadphase();
	// create the constraint solver
	m_pSolver = new btSequentialImpulseConstraintSolver();
	// create the world
	m_pWorld = new btDiscreteDynamicsWorld(m_pDispatcher, m_pBroadphase, m_pSolver, m_pCollisionConfiguration);
	//m_pWorld = new btDiscreteDynamicsWorld(m_pCollisionDispatcher, m_pBroadphase, m_pSolver, m_pCollisionConfiguration);

	m_pWorld->setInternalTickCallback(InternalPostTickCallback, 0, false);
	m_pWorld->setInternalTickCallback(InternalPreTickCallback, 0, true);
	m_pWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

	//CreateGround(btVector3(0, 0, 0));
	CreateBodies();

	Reset();

	//m_pWorld->getPairCache()->setOverlapFilterCallback(ContactManager::GetInstance().GetFilterCallback());

	using namespace std::placeholders;

	m_pDispatcher->setNearCallback(ContactManager::MyNearCallback);

	LoadTextures();

	m_collisionClock.reset();

	// Initialize the Database
	InitializeDB();
	/* Threading */
	// m_Ready = true;
	m_dbSaver = std::thread(std::bind(&ContactLearningApp::DBWorkerThread, this));
	//m_Processed = true;

	// Reset
	m_restart = false;

}

void ContactLearningApp::Idle() {
	BulletOpenGLApplication::Idle();
}

void ContactLearningApp::LoadTextures() {
	// Load up the textures
	m_ground_texture = SOIL_load_OGL_texture
		(
		"..\\..\\Dependencies\\Resources\\checkerboard.png",
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);

	if (0 == m_ground_texture)
	{
		printf("SOIL loading error: '%s'\n", SOIL_last_result());
	}

}

void ContactLearningApp::CreateGround(const btVector3 &pos) {
	// Create 3D ground.
	GameObject *ground = CreateGameObject(new btBoxShape(btVector3(GROUND_WIDTH, GROUND_HEIGHT, GROUND_DEPTH)), 0, btVector3(1.0f, 1.0f, 1.0f), btVector3(0,0,0), "Ground init");
	ground->GetShape()->setUserPointer((uintptr_t) GROUND);
	ContactManager::GetInstance().AddObjectToCollideWith(ground);

	btTransform tr;
	ground->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	btVector3 localCoordinate = btVector3(GROUND_WIDTH, 0, 0);
	
	m_groundEndPt = tr * localCoordinate;

	printf("endpt = (%f, %f, %f)\n", m_groundEndPt.x(), m_groundEndPt.y(), m_groundEndPt.z());

	m_grounds.push_back(ground);
	CreateMoarTerrain();

}

void ContactLearningApp::CreateBodies() {
	// Add the rag doll
	m_ragDoll = std::unique_ptr<RagDoll>(new RagDoll(this, true));
	//m_ragDoll = std::unique_ptr<RagDoll>(new RagDoll(this, false));
	m_ragDoll->InitializeRagDoll(GROUND_HEIGHT, m_main_window_id);

	m_ragDoll->m_ResetCallback = std::bind(&ContactLearningApp::Reset, this);
	m_ragDoll->m_StartCallback = std::bind(&ContactLearningApp::Start, this);
	m_ragDoll->m_SimulationEnd = std::bind(&ContactLearningApp::SimulationEnd, this);

	m_running = false;

}

void ContactLearningApp::ManageTerrainCreation(){

	// Only delete grounds up to the bipeds current standing ground.
	
	if (m_generationClock.getTimeMilliseconds() > 500) {

		btVector3 ragDollLocation = m_cameraManager->GetCameraLocation();
		//printf("target.x= %f \n", target.x());
		if (ragDollLocation.x() > m_groundEndPt.x() - 1.3f) {
			CreateMoarTerrain();
		}

		while (m_grounds.size() > 6) {
			GameObject *currentGround = GetCurrentStandingGround();
			GameObject *ground = m_grounds.front();
			if (ground == currentGround) {
				break;
			}
			else {
				m_grounds.pop_front();
				m_ground_idx -= 1;
				// Remove from world
				m_pWorld->removeRigidBody(ground->GetRigidBody());
				// Remove from world objects
				m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), ground), m_objects.end());
			}
		}
		m_generationClock.reset();
	}

}

void ContactLearningApp::CreateMoarTerrain() {
	std::vector<GameObject*> grounds = tc->CreateTerrains(m_groundEndPt);


	for (auto const & value : grounds) {
		m_objects.push_back(value);
		value->GetShape()->setUserPointer((uintptr_t)GROUND);

		// check if the world object is valid
		if (m_pWorld) {
			// add the object's rigid body to the world
			m_pWorld->addRigidBody(value->GetRigidBody());
		}

		m_grounds.push_back(value);
	}

	GameObject *lastObject = grounds.back();
	btTransform tr;
	lastObject->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	m_groundEndPt = tr(btVector3(GROUND_WIDTH, 0, 0));
	printf("new ground end pt = %f \n", m_groundEndPt.x());
}

void ContactLearningApp::ManageGroundCollisions() {
	
	if (m_ragDoll->GetLocation().x() > m_collisionEndPt.x() - 0.6f) {

		for (int i = 0; i < 3; i++, m_ground_idx ++) {

			if (m_ground_idx > m_grounds.size() - 1) {
				break;
			}
			else {
				m_collisionGrounds.push_back(m_grounds.at(m_ground_idx));
				//ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(m_ground_idx), 3.0f);
				// Generate number between 1000 and 3000
				int rand_stiffness = rand() % 2000 + 1000;
				float rand_damping = (float)rand_stiffness / 10.0f;
				printf("stiff = %d, damp = %f \n", rand_stiffness, rand_damping);
				ContactManager::GetInstance().AddGroundToCollideWith(
					m_grounds.at(m_ground_idx), 
					(float) rand_stiffness, 
					(float) rand_damping, 
					3.0f);
			}
		}
		while (m_collisionGrounds.size() > 5) {
			GameObject *ground = m_collisionGrounds.front();
			m_collisionGrounds.pop_front();
			ContactManager::GetInstance().RemoveObjectToCollideWith(ground);
		}

		btTransform tr;
		m_collisionGrounds.back()->GetRigidBody()->getMotionState()->getWorldTransform(tr);

		m_collisionEndPt = tr(btVector3(GROUND_WIDTH, 0, 0));
	}
}

GameObject *ContactLearningApp::GetCurrentStandingGround() {
	for (auto ground : m_collisionGrounds) {
		btTransform tr = ground->GetRigidBody()->getWorldTransform().inverse();
		btVector3 relPos = tr(m_ragDoll->GetLocation());

		if (relPos.x() >= -GROUND_WIDTH && relPos.x() < GROUND_WIDTH) {
			//printf("Found Ground. \n");
			return ground;
		}
	}
	return nullptr;
}

void ContactLearningApp::Reset() {
	
	m_collisionGrounds.clear();
	ContactManager::GetInstance().ClearObjectsToCollideWith();

	// Remove grounds start with newly generated grounds.
	for (int i = 0; i < m_grounds.size(); i ++) {
		GameObject *ground = m_grounds.at(i);
		// Remove from world
		m_pWorld->removeRigidBody(ground->GetRigidBody());
		// Remove from world objects
		m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), ground), m_objects.end());
	}

	m_grounds.clear();

	CreateGround();

	m_order = 0;
	m_sequenceNumber += 1;

	for (m_ground_idx = 0; m_ground_idx < 3; m_ground_idx++) {
		m_collisionGrounds.push_back(m_grounds.at(m_ground_idx));
		//ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(m_ground_idx), 3.0f);
		int rand_stiffness = rand() % 2000 + 1000;
		float rand_damping = (float)rand_stiffness / 10.0f;
		printf("stiff = %d, damp = %f \n", rand_stiffness, rand_damping);
		ContactManager::GetInstance().AddGroundToCollideWith(
			m_grounds.at(m_ground_idx),
			(float)rand_stiffness,
			(float)rand_damping,
			3.0f);
	}

	btTransform tr;
	m_collisionGrounds.back()->GetRigidBody()->getMotionState()->getWorldTransform(tr);

	m_collisionEndPt = tr(btVector3(GROUND_WIDTH, 0, 0));

	m_running = false;

	m_ragDoll->AddCollisionWithGround(m_collisionGrounds.front());

}

void ContactLearningApp::Start() {

	m_sampleClock.reset();
	m_collisionClock.reset();
	m_running = true;

}

void ContactLearningApp::SimulationEnd() {

	// Reset the simulation.
	m_ragDoll->Reset();
	m_resetTimerClock.reset();
	// Restart the simluation
	//m_ragDoll->Start();
	m_restart = true;
}

#pragma endregion INIT

#pragma region DRAWING

void ContactLearningApp::DrawShapeCallback(btScalar *transform, const btCollisionShape *shape, const btVector3 &color) {


	if (shape->getUserPointer() == (uintptr_t)GROUND)
	{
		// Transform to location
		const btBoxShape *box = static_cast<const btBoxShape*>(shape);
		btVector3 halfSize = box->getHalfExtentsWithMargin();

		glColor3f(color.x(), color.y(), color.z());

		// push the matrix stack
		glPushMatrix();
		glMultMatrixf(transform);

		// BulletOpenGLApplication::DrawShape(transform, shape, color);
		glMatrixMode(GL_MODELVIEW);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_ground_texture);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);								glVertex3f(-halfSize.x(), -halfSize.y(), halfSize.z());
		glTexCoord2d(0.0, halfSize.y() * 2);				glVertex3f(-halfSize.x(), halfSize.y(), halfSize.z());
		glTexCoord2d(halfSize.x() * 2, halfSize.y() * 2);	glVertex3f(halfSize.x(), halfSize.y(), halfSize.z());
		glTexCoord2d(halfSize.x() * 2, 0.0f);				glVertex3f(halfSize.x(), -halfSize.y(), halfSize.z());
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2d(0.0, 0.0);										glVertex3f(-halfSize.x(), halfSize.y(), halfSize.z());
		glTexCoord2d(halfSize.x(), 0);							glVertex3f(halfSize.x(), halfSize.y(), halfSize.z());
		glTexCoord2d(halfSize.x(), halfSize.z() * 2);			glVertex3f(halfSize.x(), halfSize.y(), -halfSize.z());
		glTexCoord2d(0, halfSize.z() * 2);							glVertex3f(-halfSize.x(), halfSize.y(), -halfSize.z());
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glPopMatrix();

	}
	else {
		if (!m_ragDoll->DrawShape(transform, shape, color)) {
			DrawShape(transform, shape, color);
		}
	}

}

void ContactLearningApp::DrawCallback() {

	ContactManager::GetInstance().DrawContactPoints();

	// Draw a circle around the ground end point

	glColor3f(1.0f, 0.0f, 0.0f);

	glPushMatrix();

	glTranslatef(m_groundEndPt.x(), m_groundEndPt.y(), m_groundEndPt.z());
	glLineWidth(0.5);
	glBegin(GL_LINES);
	glVertex3f(0, -20, 0);
	glVertex3f(0, 20, 0);
	glEnd();
	DrawCircle(0.1f);
	glPopMatrix();

	glColor3f(0.0f, 0.0f, 1.0f);

	glPushMatrix();

	glTranslatef(m_collisionEndPt.x(), m_collisionEndPt.y(), m_collisionEndPt.z());
	glLineWidth(0.5);
	glBegin(GL_LINES);
	glVertex3f(0, -20, 0);
	glVertex3f(0, 20, 0);
	glEnd();
	DrawCircle(0.1f);
	glPopMatrix();

}

#pragma endregion DRAWING

#pragma region SIM_CALLBACKS

void ContactLearningApp::PostTickCallback(btScalar timestep) {

	if (m_running) {

		if (m_collisionClock.getTimeMilliseconds() > 200) {
			for (auto ground : m_collisionGrounds) {
				btTransform tr = ground->GetRigidBody()->getWorldTransform().inverse();
				btVector3 relPos = tr(m_ragDoll->GetLocation());
				if (relPos.x() > -GROUND_WIDTH - 0.65f && relPos.x() < GROUND_WIDTH + 0.65f) {
					m_ragDoll->AddCollisionWithGround(ground);
				}
				else {
					m_ragDoll->RemoveCollisionWithGround(ground);
				}
			}
			m_collisionClock.reset();
		}

		// Check for collisions
		m_ragDoll->RagDollCollision(timestep, m_pWorld, m_collisionGrounds);

		// Terrain Creation
		ManageTerrainCreation();

		// Collision Queue Manager
		ManageGroundCollisions();

		// Check the sample clock for sampling
		if (m_sampleClock.getTimeMilliseconds() > 500) {
			if (ContactManager::GetInstance().m_beingUsed) {
				SQL_DataWrapper dat = PushDataIntoQueue();

				if (dat.m_GROUND_STIFFNESS != 0.0f) {
					std::unique_lock <std::mutex> l(m_mutex);
					m_data.push_front(dat);
					m_NotEmptyCV.notify_one();
					l.unlock();
				}

			}
			m_sampleClock.reset();
		}
	}

	if (m_restart && m_resetTimerClock.getTimeMilliseconds() > 700) {
		m_ragDoll->Start();
		m_restart = false;
	}

	// Follow the RagDoll
	m_cameraManager->SetCameraLocationX(m_ragDoll->GetLocation().x());
}

void ContactLearningApp::PreTickCallback(btScalar timestep) {

	ContactManager::GetInstance().Update(timestep);
	m_ragDoll->Loop();
}

#pragma region SIM_CALLBACKS

#pragma region DATABASE

void ContactLearningApp::InitializeDB() {

	// Create database
	int rc = sqlite3_open(db_path.c_str(), &m_samplesdb);
	if (rc) {
		printf("Can't open database. \n");
	}
	else {
		printf("Database successfully opened. \n");
	}

	// Create Table for storing state and targets
	char *sql_stmt;
	char *zErrorMsg;
	sql_stmt = "CREATE TABLE IF NOT EXISTS STATES("	\
		"ID INTEGER PRIMARY KEY NOT NULL," \
		"TORSO_LV_X REAL NOT NULL, " \
		"TORSO_LV_Y REAL NOT NULL," \
		"TORSO_D REAL NOT NULL," \
		"TORSO_O REAL NOT NULL," \
		"TORSO_AV REAL NOT NULL, " \
		"URL_D REAL NOT NULL," \
		"URL_O REAL NOT NULL, " \
		"URL_AV REAL NOT NULL, " \
		"ULL_D REAL NOT NULL," \
		"ULL_O REAL NOT NULL, " \
		"ULL_AV REAL NOT NULL, " \
		"LRL_D REAL NOT NULL," \
		"LRL_O REAL NOT NULL, " \
		"LRL_AV REAL NOT NULL, " \
		"LLL_D REAL NOT NULL," \
		"LLL_O REAL NOT NULL, " \
		"LLL_AV REAL NOT NULL, " \
		"RF_D REAL NOT NULL," \
		"RF_O REAL NOT NULL, " \
		"RF_AV REAL NOT NULL, " \
		"LF_D REAL NOT NULL," \
		"LF_O REAL NOT NULL, " \
		"LF_AV REAL NOT NULL, " \
		"RF_FORCES TEXT NOT NULL, " \
		"LF_FORCES TEXT NOT NULL, " \
		"GROUND_STIFFNESS, " \
		"GROUND_DAMPING, " \
		"GROUND_SLOPE); ";

	rc = sqlite3_exec(m_samplesdb, sql_stmt, NULL, 0, &zErrorMsg);
	if (rc != SQLITE_OK) {
		printf("Sql error: %s \n", zErrorMsg);
	}
	else {
		printf("States Table created successfully\n");
	}
	// Create table for storing the sequences

	sql_stmt = "CREATE TABLE IF NOT EXISTS SEQUENCES("	\
		"SEQUENCE_ID INTEGER NOT NULL, " \
		"STATE_ID INTEGER NOT NULL," \
		"SEQ_ORDER INTEGER NOT NULL); ";

	rc = sqlite3_exec(m_samplesdb, sql_stmt, NULL, 0, &zErrorMsg);

	if (rc != SQLITE_OK) {
		printf("Sql error: %s \n", zErrorMsg);
	}
	else {
		printf("Sequence Table created successfully\n");
	}

	// Query for the Number of sequences 
	const char *sql_cmd = "SELECT COALESCE(MAX(SEQUENCE_ID), 0) AS NumSequences FROM SEQUENCES;";
	char *zErrMsg = 0;
	rc = sqlite3_exec(m_samplesdb, sql_cmd, NumSequencesCallback, (void *)0, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("Error num sequences msg: %s \n", zErrMsg);
	}

}

void ContactLearningApp::SaveSamplesToDB(SQL_DataWrapper data) {
	
	btVector3 torsoLV = data.m_TORSO_LV;
	auto DsOsAVs = data.m_DsOsAVS;

	// Save data into database
	char *zErrMsg = 0;
	int rc;

	sqlite3_stmt *pStmt;
	rc = sqlite3_prepare(m_samplesdb, m_insert_states_sql_cmd, -1, &pStmt, 0);

	if (rc != SQLITE_OK) {
		printf("Insert into states prepare response: %s \n", sqlite3_errstr(rc));
	}

	int bind_idx = 1;

	sqlite3_bind_double(pStmt, bind_idx ++, data.m_TORSO_LV.x());
	sqlite3_bind_double(pStmt, bind_idx++, data.m_TORSO_LV.y());

	// BIND Distances Orientations and Angular Velocities
	for (auto tuple : DsOsAVs) {
		float distance = std::get<0>(tuple);
		float orientation = std::get<1>(tuple);
		float AV = std::get<2>(tuple);
		sqlite3_bind_double(pStmt, bind_idx++, distance);
		sqlite3_bind_double(pStmt, bind_idx++, orientation);
		sqlite3_bind_double(pStmt, bind_idx++, AV);
	}

	auto forces = m_ragDoll->GetContactForces();
	auto rightFootForces = std::get<0>(forces);
	auto leftFootForces = std::get<1>(forces);

	std::string rf_forces_str;
	std::string lf_forces_str;
	
	BuildStringFromForces(rf_forces_str, rightFootForces);
	BuildStringFromForces(lf_forces_str, leftFootForces);

	sqlite3_bind_text(pStmt, bind_idx++, rf_forces_str.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(pStmt, bind_idx++, lf_forces_str.c_str(), -1, SQLITE_TRANSIENT);

	//printf("Save stiffness = %f, damping = %f, slope = %f \n", data.m_GROUND_STIFFNESS, data.m_GROUND_DAMPING, data.m_GROUND_SLOPE);

	sqlite3_bind_double(pStmt, bind_idx++, data.m_GROUND_STIFFNESS);
	sqlite3_bind_double(pStmt, bind_idx++, data.m_GROUND_DAMPING);
	sqlite3_bind_double(pStmt, bind_idx++, data.m_GROUND_SLOPE);

	rc = sqlite3_step(pStmt);

	if (rc != 101) {
		printf("Insert into states step response: %s \n", sqlite3_errstr(rc));
	}

	sqlite3_finalize(pStmt);

	// Get the most recent ID inserted
	int rowId = sqlite3_last_insert_rowid(m_samplesdb);
	printf("row ID: %d\n", rowId);
	SaveSequenceToDB(data.m_SEQUENCE_ID, rowId, data.m_SEQ_ORDER);

}

void ContactLearningApp::SaveSequenceToDB(int sequenceNumber, int rowId, int order) {

	char *zErrMsg = 0;
	int rc;
	sqlite3_stmt *pStmt;
	rc = sqlite3_prepare(m_samplesdb, m_insert_sequences_cmd, -1, &pStmt, 0);

	sqlite3_bind_int(pStmt, 1, sequenceNumber);
	sqlite3_bind_int(pStmt, 2, rowId);
	sqlite3_bind_int(pStmt, 3, order);

	rc = sqlite3_step(pStmt);
	if (rc != 101) {
		printf("Sequences response: %s\n", sqlite3_errstr(rc));
	}
	sqlite3_finalize(pStmt);

}

void ContactLearningApp::DBWorkerThread() {

	printf("Start worker thread. \n");
	while (true) {
		//printf("Inside while loop and waiting. \n");
		std::unique_lock<std::mutex> ul(m_mutex);
		while (m_data.size() <= 0) {
			m_NotEmptyCV.wait(ul);
		}
		SQL_DataWrapper dat = m_data.back();
		m_data.pop_back();
		ul.unlock();
		//m_ReadyCV.wait(ul, [this] () {return m_Ready; });
		//printf("Condition passed. \n");
		//m_Processed = false;
		//std::cout << "Worker thread processing data. " << std::endl;
		SaveSamplesToDB(dat);
		//m_Processed = true;
		//m_Ready = false;
		//ul.unlock();
		//m_ProcessedCV.notify_one();
	}

}

SQL_DataWrapper ContactLearningApp::PushDataIntoQueue() {

	btVector3 torsoLV = m_ragDoll->GetTorsoLinearVelocity();

	std::vector<std::tuple<float, float, float>> DsOsAVs = m_ragDoll->GetDsOsAVs();
	auto forces = m_ragDoll->GetContactForces();

	// Determine which ground Rag Doll is over.
	float stiffness = 0.0f;
	float damping = 0.0f;
	float slope = 0.0f;

	for (auto ground : m_collisionGrounds) {
		btTransform tr = ground->GetRigidBody()->getWorldTransform().inverse();
		btVector3 relPos = tr(m_ragDoll->GetLocation());

		if (relPos.x() >= -GROUND_WIDTH && relPos.x() < GROUND_WIDTH) {
			//printf("Found Ground. \n");
			CollideeObject obj = ContactManager::GetInstance().m_toCollideWith.find(ground)->second;
			std::tuple<float, float> gps = obj.GetGroundProperties();
			stiffness = std::get<0>(gps);
			damping = std::get<1>(gps);
			slope = obj.m_object->GetOrientation();
			//printf("stiffness = %f, damping = %f, slope = %f \n", stiffness, damping, slope);
			break;
		}
	}

	//printf("stiffness = %f, damping = %f, slope = %f \n", stiffness, damping, slope);


	SQL_DataWrapper dat(
		torsoLV,
		DsOsAVs,
		std::get<0>(forces),
		std::get<1>(forces),
		stiffness,
		damping,
		slope,
		m_sequenceNumber,
		m_order++
		);

	return dat;
}

#pragma endregion DATABASE

#pragma region BULLET

void InternalPostTickCallback(btDynamicsWorld *world, btScalar timestep) {
	m_app->PostTickCallback(timestep);
}

void InternalPreTickCallback(btDynamicsWorld *world, btScalar timestep) {
	m_app->PreTickCallback(timestep);
}

static void ContactLearningIdle() {
	m_app->Idle();
}

static ContactLearningApp* GetInstance() {
	return m_app;
}

#pragma endregion BULLET

#pragma region SQL_CALLBACKS

static int NumSequencesCallback(void *data, int argc, char **argv, char **azColName) {
	for (int i = 0; i < argc; i++) {
		std::string col = azColName[i];
		std::string val = argv[i];

		if (!col.compare("NumSequences")) {
			printf("NumSequences = %d\n", std::stoi(val));
			m_app->m_sequenceNumber = std::stoi(val);
			m_app->m_sequenceNumber++;
		}
	}
	return 0;
}

static int NewestIDCallback(void *data, int argc, char **argv, char **azColName) {
	for (int i = 0; i < argc; i++) {
		std::string col = azColName[i];
		std::string val = argv[i];

		if (!col.compare("NewestID")) {
			printf("NewstID = %s\n", val);
			m_app->m_newestID = std::stoi(val);
		}
	}
	return 0;
}

#pragma endregion SQL_CALLBACKS