#pragma once

#include "BulletOpenGLApplication.h"
#include "btBulletDynamicsCommon.h"

class BasicTest : public BulletOpenGLApplication
{
public:

	BasicTest();
	BasicTest(ProjectionMode projectionType);
	~BasicTest();

	virtual void InitializePhysics() override;
	virtual void ShutdownPhysics() override;

	void CreateObjects();

};

