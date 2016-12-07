#pragma once
#include "BulletOpenGLApplication.h"

class SegmentedSensorTestApp :
	public BulletOpenGLApplication
{
public:
	SegmentedSensorTestApp();
	~SegmentedSensorTestApp();

	SegmentedSensorTestApp(ProjectionMode mode);

	virtual void InitializePhysics() override;
	virtual void ShutdownPhysics() override;

	virtual void Keyboard(unsigned char key, int x, int y) override;
	virtual void KeyboardUp(unsigned char key, int x, int y) override;

	void LoadTextures();
	void CreateGround();
	void CreateBodies();

	void DrawShapeCallback(btScalar *transform, const btCollisionShape *shape, const btVector3 &color);
	void DrawCallback();
	void PostTickCallback(btScalar timestep);
	void PreTickCallback(btScalar timestep);

	void DrawArrow(const btVector3 &pointOfContact, TranslateDirection direction);

private:

	GameObject *m_ground;
	GLuint m_ground_texture;

	GameObject *m_unsegmented_body;
	std::vector<GameObject*> m_segmented_body;

	bool m_drawForwardForceOnUnsegmented = false;
	bool m_drawBackwardForceOnUnsegmented = false;

	bool m_drawForwardForceOnSegmented = false;
	bool m_drawBackwardForceOnSegmented = false;

};

void InternalPostTickCallback(btDynamicsWorld *world, btScalar timestep);
void InternalPreTickCallback(btDynamicsWorld *world, btScalar timestep);

