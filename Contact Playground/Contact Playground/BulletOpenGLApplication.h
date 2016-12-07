#ifndef _BULLETOPENGLAPP_H_
#define _BULLETOPENGLAPP_H_

#include <Windows.h>
#include <gl\GL.h>
#include <freeglut\freeglut.h>
#include <functional>

#include "BulletDynamics\Dynamics\btDynamicsWorld.h"

// Includes for 2D Boxes and collision between 2D Boxes
#include "BulletCollision\CollisionShapes\btBox2dShape.h"
#include "BulletCollision\CollisionDispatch\btBox2dBox2dCollisionAlgorithm.h"
#include "BulletCollision\CollisionDispatch\btGhostObject.h"

// Our custom debug renderer
#include "DebugDrawer.h"

// Includes a custom motion state object
#include "OpenGLMotionState.h"
#include "CameraManager.h"

// Constants
#include "Constants.h"

#include "GameObject.h"
#include <vector>
#include <memory>

#define RENDER_TIME_STEP 0.02f // 20 ms = 50fps

//Debug
//#define USEDEBUG

#ifdef USEDEBUG
//#define Debug( x ) std::cout << x << std::endl
#else
#define Debug( x ) 
#endif

// Realtime
#define REALTIME

#ifdef REALTIME
#define BULLET_TIME_STEP 0.0005f // 2khz
//#define BULLET_TIME_STEP 0.002f // 500 hz

#else
#define BULLET_TIME_STEP 0.0005f // 1 ms
#endif

class DebugDrawer;

typedef std::vector<GameObject*> GameObjects; // GameObjects is a data type for storing game objects

class BulletOpenGLApplication
{

public:

	BulletOpenGLApplication();
	BulletOpenGLApplication::BulletOpenGLApplication(
		ProjectionMode mode,
		bool isFrameRateFixed = false,
		const btVector3 &target = btVector3(0, 0, 0),
		float distance = 10.0f,
		float pitch = 10.0f,
		float yaw = 0.0f,
		float cameraPosX = 0.0f,
		float cameraPosY = 0.0f
		);

	~BulletOpenGLApplication();

	void Initialize();

	int m_main_window_id = 0;

	// FreeGLUT callbacks //
	virtual void Keyboard(unsigned char key, int x, int y);
	virtual void KeyboardUp(unsigned char key, int x, int y);
	virtual void Special(int key, int x, int y);
	virtual void SpecialUp(int key, int x, int y);
	virtual void Reshape(int w, int h);
	virtual void Idle();
	virtual void Mouse(int button, int state, int x, int y);
	virtual void PassiveMotion(int x, int y);
	virtual void Motion(int x, int y);
	virtual void Display();
	virtual void GLUTTimerFunc(int value);
	virtual void DrawShape(btScalar *transform, const btCollisionShape *pShape, const btVector3 &color);

	// rendering. Can be overrideen by derived classes
	virtual void RenderScene();

	// scene updating. Can be overridden by derived classes
	virtual void UpdateScene(float dt);

	// physics functions. Can be overridden by derived classes (like BasicDemo)
	virtual void InitializePhysics() {};
	virtual void ShutdownPhysics() {};

	void SetScreenWidth(int width);
	void SetScreenHeight(int height);

	// Object Functions

	btHingeConstraint *AddHingeConstraint(
		GameObject *obj1,
		GameObject *obj2,
		const btVector3 &pivot1,
		const btVector3 &pivot2,
		const btVector3 &axis1,
		const btVector3 &axis2,
		btScalar lowLimit,
		btScalar highLimit);

	btFixedConstraint *AddFixedConstraint(
		GameObject *obj1,
		GameObject *obj2,
		const btTransform &trans1 = btTransform(btQuaternion(0, 0, 0, 1)),
		const btTransform &trans2 = btTransform(btQuaternion(0, 0, 0, 1))
		);

	void ApplyTorque(GameObject *object, const btVector3 &torque);

	GameObject *CreateGameObject(
		btCollisionShape *pShape,
		const float &mass,
		const btVector3 &color = btVector3(1.0f, 1.0f, 1.0f),
		const btVector3 &initialPosition = btVector3(0.0f, 0.0f, 0.0f),
		const btQuaternion &initialRotation = btQuaternion(0, 0, 0, 1)
		);

	GameObject *AddGameObject(GameObject *obj);

	// Callback for drawing
	std::function<void()> m_DrawCallback;
	std::function<void(btScalar *, const btCollisionShape *, const btVector3 &)> m_DrawShapeCallback;

	float m_DeltaGlutTime;
	float m_DeltaSimTime;

	btDynamicsWorld *GetWorld();

protected:

	// core Bullet Components
	btBroadphaseInterface *m_pBroadphase;
	btCollisionConfiguration *m_pCollisionConfiguration;
	btCollisionDispatcher *m_pDispatcher;
	btConstraintSolver *m_pSolver;
	btDynamicsWorld *m_pWorld;

	// clock for counting time
	btClock m_clock;
	// clock for timing simulation
	btClock m_SimClock;
	// Array for game objects
	GameObjects m_objects;

	// Camera Manager
	CameraManager *m_cameraManager;

	// Debugging
	// debug renderer
	DebugDrawer* m_pDebugDrawer;

	bool m_IsFrameRateFixed;
	float m_RemainingTime = 0.0f;

};

// Drawing Functions
void DrawBox(const btVector3 &halfSize);
void DrawPlane(const btVector3 &halfSize);
void DrawCylinderX(const btVector3 &halfExtents);
void DrawCylinderZ(const btVector3 &halfExtents);
void DrawCylinder(float radius, float halfLength, int slices);
//void DrawCircle(const float &radius);
void DrawCircle(const float &radius, const btVector3 &location, const btVector3 &color = btVector3(1.0f, 1.0f, 1.0f));
void DrawWithTriangles(const btVector3 * vertices, const int *indices, int numberOfIndices);
//void DisplayText(float x, float y, const btVector3 &color, const char *string);

static BulletOpenGLApplication *m_BulletApp;

static void GLUTTimerCallback(int value) {
	m_BulletApp->GLUTTimerFunc(value);
}

#endif
