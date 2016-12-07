#include "stdafx.h"
#include "SegmentedSensorTestApp.h"
#include <SOIL\src\SOIL.h>

using namespace std::placeholders;

enum Body {Ground = 0};

static SegmentedSensorTestApp *m_app;

SegmentedSensorTestApp::SegmentedSensorTestApp()
{
}

SegmentedSensorTestApp::~SegmentedSensorTestApp()
{
}

SegmentedSensorTestApp::SegmentedSensorTestApp(ProjectionMode mode):BulletOpenGLApplication(mode)
{
	m_app = this;
	m_DrawShapeCallback = std::bind(&SegmentedSensorTestApp::DrawShapeCallback, this, _1, _2, _3);
	m_DrawCallback = std::bind(&SegmentedSensorTestApp::DrawCallback, this);

	m_drawForwardForceOnUnsegmented = false;
	m_drawBackwardForceOnUnsegmented = false;

	m_drawForwardForceOnSegmented = false;
	m_drawBackwardForceOnSegmented = false;

}

void SegmentedSensorTestApp::ShutdownPhysics() {

	delete m_pWorld;
	delete m_pSolver;
	delete m_pBroadphase;
	delete m_pDispatcher;
	delete m_pCollisionConfiguration;

}

void SegmentedSensorTestApp::InitializePhysics() {

	// create the collision configuration
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
	// create the dispatcher
	m_pDispatcher = new btCollisionDispatcher(m_pCollisionConfiguration);

	// Adding for 2D collisions and solving
	m_pDispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, new btBox2dBox2dCollisionAlgorithm::CreateFunc());

	// create the broadphase
	m_pBroadphase = new btDbvtBroadphase();
	// create the constraint solver
	m_pSolver = new btSequentialImpulseConstraintSolver();
	// create the world
	m_pWorld = new btDiscreteDynamicsWorld(m_pDispatcher, m_pBroadphase, m_pSolver, m_pCollisionConfiguration);

	m_pWorld->setInternalTickCallback(InternalPostTickCallback, 0, false);
	m_pWorld->setInternalTickCallback(InternalPreTickCallback, 0, true);

	LoadTextures();

	CreateGround();

	CreateBodies();

}

void SegmentedSensorTestApp::Keyboard(unsigned char key, int x, int y) {

	BulletOpenGLApplication::Keyboard(key, x, y);

	switch (key)
	{
	case 'y': {
		// forward force on unsegmented
		m_drawForwardForceOnUnsegmented = true;
		m_drawBackwardForceOnUnsegmented = false;
	}
		break;
	case 'u': {
		// backward force on unsegmented
		m_drawForwardForceOnUnsegmented = false;
		m_drawBackwardForceOnUnsegmented = true;
	}
		break;
	case 'i': {
		// forward force on segmented
		m_drawForwardForceOnSegmented = true;
		m_drawBackwardForceOnSegmented = false;
	}
		break;
	case 'o': {
		// backward force on segmented
		m_drawForwardForceOnSegmented = false;
		m_drawBackwardForceOnSegmented = true;
	}
		break;
	default:
		break;
	}
}

void SegmentedSensorTestApp::KeyboardUp(unsigned char key, int x, int y) {

	BulletOpenGLApplication::KeyboardUp(key, x, y);

	switch (key)
	{
	case 'y': {
		// forward force on unsegmented
		m_drawForwardForceOnUnsegmented = false;
	}
		break;
	case 'u': {
		// backward force on unsegmented
		m_drawBackwardForceOnUnsegmented = false;
	}
		break;
	case 'i': {
		// forward force on segmented
		m_drawForwardForceOnSegmented = false;
	}
		break;
	case 'o': {
		// backward force on segmented
		m_drawBackwardForceOnSegmented = false;
	}
	default:
		break;
	}
}

void SegmentedSensorTestApp::LoadTextures() {
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

void SegmentedSensorTestApp::CreateGround() {

	// Create 3D ground.
	m_ground = CreateGameObject(new btBoxShape(btVector3(15, 0.5, 15)), 0, btVector3(1.0f, 1.0f, 1.0f));
	m_ground->GetShape()->setUserPointer(m_ground);

}

void SegmentedSensorTestApp::CreateBodies() {

	m_unsegmented_body = CreateGameObject(new btBox2dShape(btVector3(2, 0.5, 0)), 16, btVector3(1.0f, 1.0f, 0.0f), btVector3(-3.0f, 2.0f, 0.0f));

	for (int i = 0; i < 4; i++) {
		GameObject *segment = CreateGameObject(new btBox2dShape(btVector3(0.5f, 0.5f, 0.0f)), 4, btVector3(0.2 + i * .2, .2 + i * .2, .2 + i * .2), btVector3(1.5+i,2.0f,0.0f));
		m_segmented_body.push_back(segment);
	}
}

#pragma region DRAWING

void SegmentedSensorTestApp::DrawShapeCallback(btScalar *transform, const btCollisionShape *shape, const btVector3 &color) {

	if (shape->getUserPointer() == m_ground)
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
		DrawShape(transform, shape, color);
	}

}

void SegmentedSensorTestApp::DrawCallback() {

	if (m_drawForwardForceOnUnsegmented) {
		const btBoxShape *box = static_cast<const btBoxShape*>(m_unsegmented_body->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		btVector3 COMPos = m_unsegmented_body->GetCOMPosition();
		DrawArrow(COMPos + btVector3(-halfSize.x(), 0, 0), RIGHT);
	}
	if (m_drawBackwardForceOnUnsegmented) {
		const btBoxShape *box = static_cast<const btBoxShape*>(m_unsegmented_body->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		btVector3 COMPos = m_unsegmented_body->GetCOMPosition();
		DrawArrow(COMPos + btVector3(halfSize.x(), 0, 0), LEFT);
	}
	if (m_drawForwardForceOnSegmented) {
		const btBoxShape *box = static_cast<const btBoxShape*>(m_segmented_body.front()->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		btVector3 COMPos = m_segmented_body.front()->GetCOMPosition();
		DrawArrow(COMPos + btVector3(-halfSize.x(), 0, 0), RIGHT);
	}
	if (m_drawBackwardForceOnSegmented) {
		const btBoxShape *box = static_cast<const btBoxShape*>(m_segmented_body.back()->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		btVector3 COMPos = m_segmented_body.back()->GetCOMPosition();
		DrawArrow(COMPos + btVector3(halfSize.x(), 0, 0), LEFT);
	}

	// Draw lines from vertices of unsegmented body
	// Transform to location of the body
	/*
	btScalar transform[16];
	m_unsegmented_body->GetTransform(transform);

	glColor3f(1.0f, 0.0f, 0.0f);
	glPushMatrix();
	glMultMatrixf(transform);
	glTranslatef(0, 0, 0.1f);

	const btBoxShape *box = static_cast<const btBoxShape*>(m_unsegmented_body->GetShape());
	btVector3 halfSize = box->getHalfExtentsWithMargin();

	// Get the points of the body
	std::vector<btVector3> points;

	for (int i = 0; i < 5; i++) {
		btVector3 point(-halfSize.x() + i * halfSize.x()/2, -halfSize.y(), 0.0f);
		points.push_back(point);
	}
	glLineWidth(2.5f);
	
	for (int i = 0; i < points.size(); i++) {
		btVector3 vb = points.at(i);
		btVector3 ve = vb + btVector3(0.5, 2, 0.0f);
		glBegin(GL_LINES);
		glVertex2f(vb.x(), vb.y());
		glVertex2f(ve.x(), ve.y());
		glEnd();
		char buf[200];
		sprintf_s(buf, "F");
		DisplayText(ve.x(), ve.y(), btVector3(1.0f, 0.0f, 0.0f), buf);
	}
	
	glPopMatrix();
	*/
}

void SegmentedSensorTestApp::DrawArrow(const btVector3 &pointOfContact, TranslateDirection direction) {

	glColor3f(255, 255, 255);
	glPushMatrix();

	static int indices[9] = {
		0, 1, 2,
		3, 4, 5,
		4, 5, 6
	};
	switch (direction)
	{
	case UP:
		break;
	case DOWN:
		break;
	case LEFT: {
		btVector3 vertices[7] = {
			btVector3(pointOfContact.x(), pointOfContact.y(), 0.0f),
			btVector3(pointOfContact.x() + 0.1, pointOfContact.y() + 0.15, 1.0f),
			btVector3(pointOfContact.x() + 0.1, pointOfContact.y() - 0.15, 1.0f),
			btVector3(pointOfContact.x() + 0.1, pointOfContact.y() + 0.05, 1.0f),
			btVector3(pointOfContact.x() + 0.1, pointOfContact.y() - 0.05, 1.0f),
			btVector3(pointOfContact.x() + 0.35, pointOfContact.y() + 0.05, 1.0f),
			btVector3(pointOfContact.x() + 0.35, pointOfContact.y() - 0.05, 1.0f),
		};
		DrawWithTriangles(vertices, indices, 9);
	}

		break;
	case RIGHT: {
		btVector3 vertices[7] = {
			btVector3(pointOfContact.x(), pointOfContact.y(), 0.0f),
			btVector3(pointOfContact.x() - 0.1, pointOfContact.y() + 0.15, 1.0f),
			btVector3(pointOfContact.x() - 0.1, pointOfContact.y() - 0.15, 1.0f),
			btVector3(pointOfContact.x() - 0.1, pointOfContact.y() + 0.05, 1.0f),
			btVector3(pointOfContact.x() - 0.1, pointOfContact.y() - 0.05, 1.0f),
			btVector3(pointOfContact.x() - 0.35, pointOfContact.y() + 0.05, 1.0f),
			btVector3(pointOfContact.x() - 0.35, pointOfContact.y() - 0.05, 1.0f),
		};
		DrawWithTriangles(vertices, indices, 9);
	}
		break;
	default:
		break;
	}
}

void SegmentedSensorTestApp::PostTickCallback(btScalar timestep) {

	int numManifolds = m_pWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold = m_pWorld->getDispatcher()->getManifoldByIndexInternal(i);

		btCollisionObject* obA = const_cast<btCollisionObject*>(contactManifold->getBody0());
		btCollisionObject* obB = const_cast<btCollisionObject*>(contactManifold->getBody1());

		for (int j = 0; j < contactManifold->getNumContacts(); j++)   {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.m_distance1 < 0) {
				// Valid contact point
				//pt.getAppliedImpulse();
				// Go to the position of the contact.
				btVector3 posA = pt.getPositionWorldOnA();
				glPushMatrix();
				glTranslatef(posA.x(), posA.y(), posA.z() + 0.1f);
				glColor3f(1.0f, 0.0f, 0.0f);
				DrawCircle(0.1f);

				glLineWidth(2.5f);

				glBegin(GL_LINES);
				glVertex2f(0.0f, 0.0f);
				glVertex2f(0.5f, 3.0f + j * 1.2f);
				glEnd();

				char buf[200];
				sprintf_s(buf, "%f", pt.getAppliedImpulse());
				DisplayText(0.5f, 3.0f + j * 1.2f, btVector3(1.0f, 0.0f, 0.0f), buf);

				glPopMatrix();
			}
		}
	}
}

void SegmentedSensorTestApp::PreTickCallback(btScalar timestep) {

	btVector3 force;
	btVector3 relPos;

	if (m_drawForwardForceOnUnsegmented)
	{
		force = btVector3(3.0f, 0.0f, 0.0f);
		const btBoxShape *box = static_cast<const btBoxShape*>(m_unsegmented_body->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		m_unsegmented_body->GetRigidBody()->applyImpulse(force, btVector3(-halfSize.x(), 0.0f, 0.0f));
	}
	if (m_drawBackwardForceOnUnsegmented) {
		force = btVector3(-3.0f, 0.0f, 0.0f);
		const btBoxShape *box = static_cast<const btBoxShape*>(m_unsegmented_body->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		m_unsegmented_body->GetRigidBody()->applyImpulse(force, btVector3(halfSize.x(), 0.0f, 0.0f));
	}
	if (m_drawForwardForceOnSegmented) {
		force = btVector3(3.0f, 0.0f, 0.0f);
		const btBoxShape *box = static_cast<const btBoxShape*>(m_segmented_body.front()->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		m_segmented_body.at(0)->GetRigidBody()->applyImpulse(force, btVector3(-halfSize.x(), 0.0f, 0.0f));
	}
	if (m_drawBackwardForceOnSegmented) {
		force = btVector3(-3.0f, 0.0f, 0.0f);
		const btBoxShape *box = static_cast<const btBoxShape*>(m_segmented_body.back()->GetShape());
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		m_segmented_body.back()->GetRigidBody()->applyImpulse(force, btVector3(halfSize.x(), 0.0f, 0.0f));
	}

}

#pragma endregion DRAWING

void InternalPostTickCallback(btDynamicsWorld *world, btScalar timestep) {
	m_app->PostTickCallback(timestep);
}

void InternalPreTickCallback(btDynamicsWorld *world, btScalar timestep) {
	m_app->PreTickCallback(timestep);
}