#include "stdafx.h"
#include <boost\asio.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>
#include "ContactLearningApp.h"
#include <SOIL\src\SOIL.h>
#include "ContactManager.h"
#include <glui\glui.h>
#include <functional>

#include <stdexcept>

#include "TerrainCreator.h"


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

	CreateGround(btVector3(0, 0, 0));
	CreateBodies();

	//m_pWorld->getPairCache()->setOverlapFilterCallback(ContactManager::GetInstance().GetFilterCallback());

	using namespace std::placeholders;

	m_pDispatcher->setNearCallback(ContactManager::MyNearCallback);

	LoadTextures();

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
	GameObject *ground = CreateGameObject(new btBoxShape(btVector3(GROUND_WIDTH, GROUND_HEIGHT, GROUND_DEPTH)), 0, btVector3(1.0f, 1.0f, 1.0f), btVector3(0,0,0));
	ground->GetShape()->setUserPointer((uintptr_t) GROUND);
	ContactManager::GetInstance().AddObjectToCollideWith(ground);

	btTransform tr;
	ground->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	btVector3 localCoordinate = btVector3(GROUND_WIDTH, 0, 0);
	
	m_groundEndPt = tr * localCoordinate;

	printf("endpt = (%f, %f, %f)\n", m_groundEndPt.x(), m_groundEndPt.y(), m_groundEndPt.z());

	m_grounds.push_back(ground);
	CreateMoarTerrain();

	Reset();

}

void ContactLearningApp::CreateBodies() {
	// Add the rag doll
	m_ragDoll = std::unique_ptr<RagDoll>(new RagDoll(this, true));
	m_ragDoll->InitializeRagDoll(GROUND_HEIGHT, m_main_window_id);

	m_ragDoll->m_ResetCallback = std::bind(&ContactLearningApp::Reset, this);

}

void ContactLearningApp::ManageTerrainCreation(){

	btVector3 ragDollLocation = m_cameraManager->GetCameraLocation();
	//printf("target.x= %f \n", target.x());
	if (ragDollLocation.x() > m_groundEndPt.x() - 1.3f) {
		CreateMoarTerrain();
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
	
	if (m_ragDoll->GetLocation().x() > m_collisionEndPt.x() - 1.0f) {

		for (int i = 0; i < 3; i++, m_ground_idx ++) {

			if (m_ground_idx > m_grounds.size() - 1) {
				break;
			}
			else {
				m_collisionGrounds.push_back(m_grounds.at(m_ground_idx));
				ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(m_ground_idx), 7.0f);
			}
		}
		if (m_collisionGrounds.size() > 3) {
			GameObject *ground = m_collisionGrounds.front();
			m_collisionGrounds.pop_front();
			ContactManager::GetInstance().RemoveObjectToCollideWith(ground);
		}

		btTransform tr;
		m_collisionGrounds.back()->GetRigidBody()->getMotionState()->getWorldTransform(tr);

		m_collisionEndPt = tr(btVector3(GROUND_WIDTH, 0, 0));
	}
}

void ContactLearningApp::Reset() {
	
	m_collisionGrounds.clear();
	ContactManager::GetInstance().ClearObjectsToCollideWith();


	for (m_ground_idx = 0; m_ground_idx < 3; m_ground_idx++) {
		m_collisionGrounds.push_back(m_grounds.at(m_ground_idx));
		ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(m_ground_idx), 5.0f);
	}

	ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(0));
	ContactManager::GetInstance().AddObjectToCollideWith(m_grounds.at(1));

	btTransform tr;
	m_collisionGrounds.back()->GetRigidBody()->getMotionState()->getWorldTransform(tr);

	m_collisionEndPt = tr(btVector3(GROUND_WIDTH, 0, 0));

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

void ContactLearningApp::PostTickCallback(btScalar timestep) {

	// Check for collisions
	m_ragDoll->RagDollCollision(timestep, m_pWorld, m_collisionGrounds);

	// Terrain Creation
	ManageTerrainCreation();

	// Collision Queue Manager
	ManageGroundCollisions();

	// Follow the RagDoll
	m_cameraManager->SetCameraLocationX(m_ragDoll->GetLocation().x());

}

void ContactLearningApp::PreTickCallback(btScalar timestep) {
	ContactManager::GetInstance().Update(timestep);
	m_ragDoll->Loop();
}

#pragma endregion DRAWING

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