#include "stdafx.h"
#include "BulletOpenGLApplication.h"
#include <iostream>

#pragma region INITIALIZATION

BulletOpenGLApplication::BulletOpenGLApplication()
{
	Debug("Constructing BulletOpenGLApplication and building camera");
	// Create Camera manager
	m_cameraManager = new CameraManager(
		btVector3(0.0f, 0.0f, 0.0f),	// Target
		10,								// Distance
		50.0f,							// Pitch
		0.0f,							// Yaw
		btVector3(0.0f, 1.0f, 0.0f),	// Up Vector
		1.0f,							// near plane
		1000.0f);						// far plane

	m_BulletApp = this;
}

BulletOpenGLApplication::BulletOpenGLApplication(ProjectionMode mode, bool isFrameRateFixed, const btVector3 &target, float distance, float pitch, float yaw, float cameraPosX, float cameraPosY) {
	Constants::GetInstance().SetProjectionMode(mode);
	m_IsFrameRateFixed = isFrameRateFixed;
	m_cameraManager = new CameraManager(
		target,
		distance,							
		pitch,							
		yaw,							
		btVector3(0.0f, 1.0f, 0.0f),	// Up Vector
		1.0f,							// near plane
		1000.0f,
		cameraPosX,
		cameraPosY);						// far plane

	m_BulletApp = this;
}

BulletOpenGLApplication::~BulletOpenGLApplication() {
	// Shutdown physics system
	ShutdownPhysics();
}

void BulletOpenGLApplication::Initialize() {
	Debug("Initializing BulletOpenGLApplication");
	// this function is called inside glutmain() after
	// creating the window, but before handing control
	// to FreeGLUT

	// create some floats for our ambient, diffuse, specular and position
	GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // dark grey
	GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat position[] = { 0.0f, 10.0f, 10.0f, 0.0f };
	//GLfloat position_1[] = { 0.0f, 10.0f, 10.0f, 0.0f };

	// set the ambient, diffuse, specular and position for LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	/*glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT1, GL_POSITION, position_1);*/

	glEnable(GL_LIGHTING); // enables lighting
	glEnable(GL_LIGHT0); // enables the 0th light
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL); // colors materials when lighting is enabled

	// enable specular lighting via materials
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMateriali(GL_FRONT, GL_SHININESS, 15);

	// enable smooth shading
	glShadeModel(GL_SMOOTH);

	// enable depth testing to be 'less than'
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Disable Culling
	glDisable(GL_CULL_FACE);

	// set the backbuffer clearing color to a lightish blue
	glClearColor(0.6, 0.65, 0.85, 0);

	// initialize the physics system
	InitializePhysics();

	// create the debug drawer
	m_pDebugDrawer = new DebugDrawer();
	// set the initial debug level to 0
	m_pDebugDrawer->setDebugMode(0);
	// add the debug drawer to the world
	m_pWorld->setDebugDrawer(m_pDebugDrawer);

	Constants::GetInstance().m_StartTime = glutGet(GLUT_ELAPSED_TIME);
	Constants::GetInstance().m_PrevTime = Constants::GetInstance().m_StartTime;

	if (m_IsFrameRateFixed) {
		glutTimerFunc((int)(RENDER_TIME_STEP * 1000), GLUTTimerCallback, 0);
	}

}

void BulletOpenGLApplication::SetScreenWidth(int width) {
	Constants::GetInstance().SetScreenWidth(width);
}

void BulletOpenGLApplication::SetScreenHeight(int height) {
	Constants::GetInstance().SetScreenHeight(height);
}

#pragma endregion INITIALIZATION

#pragma region GLUT_CALLBACKS

#pragma region KEYBOARD

void BulletOpenGLApplication::Keyboard(unsigned char key, int x, int y) {
	// This function is called by FreeGLUT whenever
	// generic keys are pressed down.
	// Common to all projection types
	switch (key)
	{
	case 'v': {
		// toggle wireframe debug drawing
		m_pDebugDrawer->ToggleDebugFlag(btIDebugDraw::DBG_DrawWireframe);
		Debug("toggle debug wireframe");
	}
		break;
	case 'b': {
		// toggle AABB debug drawing
		m_pDebugDrawer->ToggleDebugFlag(btIDebugDraw::DBG_DrawAabb);
		Debug("toggle debug flag");
	}
		break;
	case 'z': m_cameraManager->ZoomCamera(+CAMERA_STEP_SIZE); break;			// 'z' zooms in
	case 'x': m_cameraManager->ZoomCamera(-CAMERA_STEP_SIZE); break;			// 'x' zoom out
	case 'w': m_cameraManager->TranslateCamera(UP, CAMERA_STEP_SIZE); break;
	case 'a': m_cameraManager->TranslateCamera(LEFT, CAMERA_STEP_SIZE); break;
	case 's': m_cameraManager->TranslateCamera(DOWN, CAMERA_STEP_SIZE); break;
	case 'd': m_cameraManager->TranslateCamera(RIGHT, CAMERA_STEP_SIZE); break;

	default:
		break;
	}

}

void BulletOpenGLApplication::KeyboardUp(unsigned char key, int x, int y) {}

void BulletOpenGLApplication::Special(int key, int x, int y) {
	// This function is called by FreeGLUT whenever special keys
	// are pressed down, like the arrow keys, or Insert, Delete etc.
	//printf("Received Special Key\n");

	switch (key) {
		// the arrow keys rotate the camera up/down/left/right
	case GLUT_KEY_LEFT:
		m_cameraManager->RotateCamera(YAW, +CAMERA_STEP_SIZE); break;
	case GLUT_KEY_RIGHT:
		m_cameraManager->RotateCamera(YAW, -CAMERA_STEP_SIZE); break;
	case GLUT_KEY_UP:
		m_cameraManager->RotateCamera(PITCH, +CAMERA_STEP_SIZE); break;
	case GLUT_KEY_DOWN:
		m_cameraManager->RotateCamera(PITCH, -CAMERA_STEP_SIZE); break;
	}

}

void BulletOpenGLApplication::SpecialUp(int key, int x, int y) {}

#pragma endregion KEYBOARD

void BulletOpenGLApplication::Reshape(int w, int h) {
	Debug("BulletOpenGLApplication Reshape called");
	// this function is called once during application intialization
	// and again every time we resize the window

	// set the viewport
	glViewport(0, 0, w, h);

	Constants::GetInstance().SetScreenWidth(w);
	Constants::GetInstance().SetScreenHeight(h);

	// update the camera
	m_cameraManager->UpdateCamera();
	//m_cameraManager->PrintCameraLocation();

}

void BulletOpenGLApplication::Idle() {

	// this function is called frequently, whenever FreeGlut
	// isn't busy processing its own events. It should be used
	// to perform any updating and rendering tasks

	if (!m_IsFrameRateFixed) {
		glutSetWindow(m_main_window_id);

		// clear the backbuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef REALTIME
		// get the time since the last iteration
		static bool firstTime = true;
		float dt = firstTime ? 0.0 : 0.001 * 0.001 * m_clock.getTimeMicroseconds();
		firstTime = false;
		m_clock.reset();

		float completeTime = dt + m_RemainingTime;

		float desiredTimeBetweenFrames = 1.0 / 60.0; // 60 frames per seconds
		int numSteps = ceil(desiredTimeBetweenFrames / BULLET_TIME_STEP); // if ???
		//int numSteps = ceil(completeTime / BULLET_TIME_STEP); // or this if desiredTimeBetweenFrames < monitor refresh rate ???
		m_RemainingTime = completeTime - numSteps * BULLET_TIME_STEP;

		if (m_RemainingTime > 3 * dt)
		{
			m_RemainingTime = 0;
		}

		while (m_RemainingTime < 0 && numSteps > 0)
		{
			numSteps -= 1;
			m_RemainingTime = completeTime - numSteps * BULLET_TIME_STEP;
		}

		if (m_pWorld) {

			// step the simulation through time. This is called
			// every update and the amount of elasped time was 
			// determined back in ::Idle() by our clock object.

			m_SimClock.reset();
			for (int i = 0; i < numSteps; i++) {
				m_pWorld->stepSimulation(BULLET_TIME_STEP, 0);
			}
			m_DeltaSimTime = m_SimClock.getTimeMilliseconds();
		}
		int m = m_clock.getTimeMicroseconds();

		float posX = m_cameraManager->GetCameraLocation().x() - 1.7;

		char buf[100];
		sprintf_s(buf, "physics computation time = %d", m);
		DisplayText(posX, 1.2, btVector3(0, 0, 0), buf);
		sprintf_s(buf, "num steps = %d", numSteps);
		DisplayText(posX, 1.14, btVector3(0, 0, 0), buf);
		sprintf_s(buf, "elapsed time = %f", dt);
		DisplayText(posX, 1.08, btVector3(0, 0, 0), buf);
		sprintf_s(buf, "remaining time = %f", m_RemainingTime);
		DisplayText(posX, 1.02, btVector3(0, 0, 0), buf);
		sprintf_s(buf, "complete time = %f", completeTime);
		DisplayText(posX, 0.96, btVector3(0, 0, 0), buf);
		//printf("camera location x = %f \n", m_cameraManager->GetCameraLocation().x());
		
		m_DeltaGlutTime = dt;
#else 
		if (m_pWorld) {

			// step the simulation through time. This is called
			// every update and the amount of elasped time was 
			// determined back in ::Idle() by our clock object.
			for (int i = 0; i < 9; i++) {
				m_pWorld->stepSimulation(BULLET_TIME_STEP, 0);
			}
		}
#endif

		m_cameraManager->UpdateCamera();

		// render the scene
		RenderScene();

		try {
			m_DrawCallback();
		}
		catch (const std::bad_function_call& e) {
			//std::cout << e.what() << '\n';
		}

		// swap the front and back buffers
		glutPostRedisplay();
		glutSwapBuffers();
	}
}

void BulletOpenGLApplication::GLUTTimerFunc(int value) {

	// Setup next timer tick 
	glutTimerFunc((int)(RENDER_TIME_STEP * 1000), GLUTTimerCallback, 0);
	glutSetWindow(m_main_window_id);

	glutPostRedisplay();

	// this function is called frequently, whenever FreeGlut
	// isn't busy processing its own events. It should be used
	// to perform any updating and rendering tasks

	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// get the time since the last iteration
	// Measure the elapsed time
	Constants::GetInstance().m_CurrentTime = glutGet(GLUT_ELAPSED_TIME);
	int timeSincePrevFrame = Constants::GetInstance().m_CurrentTime - Constants::GetInstance().m_PrevTime; // milliseconds
	char buf[100];
	sprintf_s(buf, "Time since previous frame: %d ms", timeSincePrevFrame);
	DisplayText(-2, 1.5, btVector3(0, 0, 0), buf);
	float dt = (float)timeSincePrevFrame / 1000.0f; // seconds
	UpdateScene(dt);

	m_cameraManager->UpdateCamera();

	// render the scene
	RenderScene();

	m_DeltaGlutTime = dt;

	try {
		m_DrawCallback();
	}
	catch (const std::bad_function_call& e) {
		//std::cout << e.what() << '\n';
	}

	// swap the front and back buffers
	//glutSwapBuffers();

	Constants::GetInstance().m_PrevTime = Constants::GetInstance().m_CurrentTime;

}

void BulletOpenGLApplication::Mouse(int button, int state, int x, int y) {}

void BulletOpenGLApplication::PassiveMotion(int x, int y) {}

void BulletOpenGLApplication::Motion(int x, int y) {}

void BulletOpenGLApplication::Display() {}

#pragma endregion GLUT_CALLBACKS

#pragma region DRAWING

void DrawBox(const btVector3 &halfSize) {
	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z();

	// Create vertex positions
	btVector3 vertices[8] = {
		btVector3(halfWidth, halfHeight, halfDepth),
		btVector3(halfWidth, halfHeight, -halfDepth),
		btVector3(halfWidth, -halfHeight, halfDepth),
		btVector3(halfWidth, -halfHeight, -halfDepth),
		btVector3(-halfWidth, halfHeight, halfDepth),
		btVector3(-halfWidth, halfHeight, -halfDepth),
		btVector3(-halfWidth, -halfHeight, halfDepth),
		btVector3(-halfWidth, -halfHeight, -halfDepth)
	};

	// create the indexes for each triangle, using the 
	// vertices above. Make it static so we don't waste 
	// processing time recreating it over and over again
	static int indices[36] = {
		0, 1, 2,
		3, 2, 1,
		4, 0, 6,
		6, 0, 2,
		5, 1, 4,
		4, 1, 0,
		7, 3, 1,
		7, 1, 5,
		5, 4, 7,
		7, 4, 6,
		7, 2, 3,
		7, 6, 2 };

	DrawWithTriangles(vertices, indices, 36);
}

void DrawPlane(const btVector3 &halfSize) {


	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z(); // No depth

	// Create Vector
	btVector3 vertices[4] = {
		btVector3(-halfWidth, -halfHeight, 0.0f),
		btVector3(-halfWidth, halfHeight, 0.0f),
		btVector3(halfWidth, -halfHeight, 0.0f),
		btVector3(halfWidth, halfHeight, 0.0f),
	};

	// create the indexes for each triangle, using the 
	// vertices above. Make it static so we don't waste 
	// processing time recreating it over and over again
	static int indices[6] = {
		0, 1, 2,
		3, 2, 1
	};

	DrawWithTriangles(vertices, indices, 6);
}

void DrawCircle(const float &radius, const btVector3 &location, const btVector3 &color) {
	
	glPushMatrix();
	glTranslatef(location.x(), location.y(), 0.0f);
	glColor3f(color.x(), color.y(), color.z());
	DrawCircle(radius);
	glPopMatrix();

}

void DrawWithTriangles(const btVector3 *vertices, const int *indices, int numberOfIndices) {

	// start processing vertices as triangles
	glBegin(GL_TRIANGLES);

	// increment the loop by 3 each time since we create a 
	// triangle with 3 vertices at a time.

	for (int i = 0; i < numberOfIndices; i += 3) {
		// get the three vertices for the triangle based
		// on the index values set above
		// use const references so we don't copy the object
		// (a good rule of thumb is to never allocate/deallocate
		// memory during *every* render/update call. This should 
		// only happen sporadically)
		const btVector3 &vert1 = vertices[indices[i]];
		const btVector3 &vert2 = vertices[indices[i + 1]];
		const btVector3 &vert3 = vertices[indices[i + 2]];

		// create a normal that is perpendicular to the 
		// face (use the cross product)
		btVector3 normal = (vert3 - vert1).cross(vert2 - vert1);
		normal.normalize();

		// set the normal for the subsequent vertices
		glNormal3f(normal.getX(), normal.getY(), normal.getZ());

		// create the vertices
		glVertex3f(vert1.x(), vert1.y(), vert1.z());
		glVertex3f(vert2.x(), vert2.y(), vert2.z());
		glVertex3f(vert3.x(), vert3.y(), vert3.z());
	}
	glEnd();
}

void DrawCylinderZ(const btVector3 &halfExtents) {

	//glRotatef(90.0f, 1, 0, 0);
	float height = halfExtents.x();
	float radius = halfExtents.y();
	glTranslatef(0, 0, -height/2);
	glutWireCylinder(radius, height, 32, 32);
	//DrawCylinder(radius, height, 32);
}

void DrawCylinderX(const btVector3 &halfExtents) {

}

void DrawCylinder(float radius, float halfLength, int slices) {
	for (int i = 0; i<slices; i++) {
		float theta = ((float)i)*2.0*PI;
		float nextTheta = ((float)i + 1)*2.0*PI;
		glBegin(GL_TRIANGLE_STRIP);
		/*vertex at middle of end */ glVertex3f(0.0, halfLength, 0.0);
		/*vertices at edges of circle*/ glVertex3f(radius*cos(theta), halfLength, radius*sin(theta));
		glVertex3f(radius*cos(nextTheta), halfLength, radius*sin(nextTheta));
		/* the same vertices at the bottom of the cylinder*/
		glVertex3f(radius*cos(nextTheta), -halfLength, radius*sin(nextTheta));
		glVertex3f(radius*cos(theta), -halfLength, radius*sin(theta));
		glVertex3f(0.0, -halfLength, 0.0);
		glEnd();
	}
}

void BulletOpenGLApplication::DrawShape(btScalar *transform, const btCollisionShape *pShape, const btVector3 &color) {

	glColor3f(color.x(), color.y(), color.z());

	// push the matrix stack

	glPushMatrix();
	glMultMatrixf(transform);

	// make a different draw call based on object type
	switch (pShape->getShapeType())
	{
	case BOX_SHAPE_PROXYTYPE: {
		// assume the shape is a box, and typecast it
		const btBoxShape *box = static_cast<const btBoxShape*>(pShape);
		// get halfSize of the box
		btVector3 halfSize = box->getHalfExtentsWithMargin();
		// draw the box
		// Don't think I should do this.. but whatever.
		if (Constants::GetInstance().GetProjectionMode() == ORTHOGRAPHIC) {
			DrawPlane(halfSize);
		}
		else {
			DrawBox(halfSize);
		}
	}
		break;
	case BOX_2D_SHAPE_PROXYTYPE: {
		// assume the shape is a 2d box (plane) and typecast it
		const btBox2dShape *plane = static_cast<const btBox2dShape*> (pShape);
		btVector3 halfSize = plane->getHalfExtentsWithMargin();
		DrawPlane(halfSize);
	}
		break;
	case SPHERE_SHAPE_PROXYTYPE: {
		// assume the shape is a 2d circle, typecast..
		const btSphereShape *sphere = static_cast<const btSphereShape*> (pShape);
		float radius = sphere->getRadius();
		DrawCircle(radius);
	}
		break;
	case CYLINDER_SHAPE_PROXYTYPE:
	{
		const btCylinderShape *cylinder = static_cast<const btCylinderShape *>(pShape);
		btVector3 halfExtents = cylinder->getHalfExtentsWithMargin();
		//printf("halfextents x = %f, y = %f, z = %f\n", halfExtents.x(), halfExtents.y(), halfExtents.z());
		if (cylinder->getUpAxis() == 2) {
			DrawCylinderZ(halfExtents);
		}
		else {
			DrawCylinderX(halfExtents);
		}
	}
		break;
	default:
		// unsupported type
		break;
	}

	glPopMatrix();

}

#pragma endregion DRAWING

#pragma region SCENE

void BulletOpenGLApplication::RenderScene() {

	// create an array of 16 floats (representing a 4x4 matrix)
	btScalar transform[16];

	// iterate through all of the objects in our world
	//printf("number of objects = %d\n", m_objects.size());
	for (GameObjects::iterator i = m_objects.begin(); i != m_objects.end(); ++i) {
		// get the object from the iterator
		GameObject* pObj = *i;

		// read the transform
		pObj->GetTransform(transform);

		// get data from the object and draw it
		try {
			m_DrawShapeCallback(transform, pObj->GetShape(), pObj->GetColor());
		}
		catch (const std::bad_function_call& e) {
			//std::cout << e.what() << '\n';
			DrawShape(transform, pObj->GetShape(), pObj->GetColor());
		}

	}

	// after rendering all game objects, perform debug rendering
	// Bullet will figure out what needs to be drawn then call to
	// our DebugDrawer class to do the rendering for us
	m_pWorld->debugDrawWorld();

}

void BulletOpenGLApplication::UpdateScene(float dt) {
	// check if the world object exists

	if (m_pWorld) {

		// step the simulation through time. This is called
		// every update and the amount of elasped time was 
		// determined back in ::Idle() by our clock object.


		// max simulation steps: .03/.002 = 15


		//m_Simclock.reset();
		m_pWorld->stepSimulation(dt, 15, BULLET_TIME_STEP);
		//m_deltasimtime = m_simclock.gettimemilliseconds();

		/*int numberOfSteps = 1000;
		m_SimClock.reset();
		for (int i = 0; i < numberOfSteps; ++i)
		{
		m_pWorld->stepSimulation(BULLET_TIME_STEP, 0);
		}
		m_DeltaSimTime = m_SimClock.getTimeMilliseconds();
		char buf[1000];
		sprintf_s(buf, "Simulation time for 1000 steps: %f", m_DeltaSimTime);
		DisplayText(-2, 1, btVector3(0, 0, 0), buf);*/
	}
}

#pragma endregion SCENE

#pragma region INTERACTION

GameObject* BulletOpenGLApplication::CreateGameObject(
	btCollisionShape *pShape,
	const float &mass,
	const btVector3 &color,
	const btVector3 &initialPosition,
	std::string name,
	const btQuaternion &initialRotation
	) {

	GameObject* pObject = new GameObject(pShape, mass, color, initialPosition, name, initialRotation );
	// push it to the back of the list

	return AddGameObject(pObject);
}

GameObject *BulletOpenGLApplication::AddGameObject(GameObject *obj) {
	switch (obj->GetShape()->getShapeType())
	{
	case BOX_2D_SHAPE_PROXYTYPE: {
		// assume the shape is a 2d box (plane) and typecast it
		btRigidBody *body = obj->GetRigidBody();
		// ASSUMPTION: Limit motion along x-y plane
		//printf("Allow in x y direction, disallow in z direction \n");
		body->setLinearFactor(btVector3(1, 1, 0));
		body->setAngularFactor(btVector3(0, 0, 1));
	}
								 break;
	case SPHERE_SHAPE_PROXYTYPE: {
		// Make sphere 2D..
		btRigidBody *body = obj->GetRigidBody();
		// ASSUMPTION: Limit motion along x-y plane
		//printf("Allow in x y direction, disallow in z direction \n");
		body->setLinearFactor(btVector3(1, 1, 0));
		body->setAngularFactor(btVector3(0, 0, 1));
	}
								 break;
	default:
		break;
	}

	m_objects.push_back(obj);
	Debug("Created Object and pushed to world");
	// check if the world object is valid
	if (m_pWorld) {
		// add the object's rigid body to the world
		m_pWorld->addRigidBody(obj->GetRigidBody());

	}

	return obj;
}

btHingeConstraint *BulletOpenGLApplication::AddHingeConstraint(
	GameObject *obj1,
	GameObject *obj2,
	const btVector3 &pivot1,
	const btVector3 &pivot2,
	const btVector3 &axis1,
	const btVector3 &axis2,
	btScalar lowLimit,
	btScalar highLimit) {

	btRigidBody *body1 = obj1->GetRigidBody();
	btRigidBody *body2 = obj2->GetRigidBody();

	btHingeConstraint *hc = new btHingeConstraint(*body1, *body2, pivot1, pivot2, axis1, axis2);
	hc->setDbgDrawSize(btScalar(5.0f));
	hc->setLimit(lowLimit, highLimit);

	if (m_pWorld) {
		m_pWorld->addConstraint(hc, true);
	}

	return hc;
}

btFixedConstraint *BulletOpenGLApplication::AddFixedConstraint(GameObject *obj1, GameObject *obj2, const btTransform &trans1, const btTransform &trans2) {

	btRigidBody *body1 = obj1->GetRigidBody();
	btRigidBody *body2 = obj2->GetRigidBody();

	btFixedConstraint *fc = new btFixedConstraint(*body1, *body2, trans1, trans2);

	if (m_pWorld) {
		m_pWorld->addConstraint(fc, true);
	}

	return fc;

}

void BulletOpenGLApplication::ApplyTorque(GameObject *object, const btVector3 &torque) {

	float torqueScaleFactor = SCALING_FACTOR * SCALING_FACTOR;

	btVector3 scaledTorque = torque * btVector3(torqueScaleFactor, torqueScaleFactor, torqueScaleFactor);

	object->ApplyTorque(scaledTorque);

}

btDynamicsWorld *BulletOpenGLApplication::GetWorld() {
	return m_pWorld;
}

#pragma endregion INTERACTION