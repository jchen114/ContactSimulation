#pragma once

#include "GameObject.h"

#include "BulletCollision\CollisionShapes\btBox2dShape.h"
#include "BulletOpenGLApplication.h"
#include <set>
#include <unordered_map>
#include <memory>

class CollideeObject;

typedef enum
{
	COLLIDER_BOX_2D_SHAPE = 0,
	COLLIDER_BOX_SHAPE
} ColliderType;

class ColliderVertex;
class CollideeObject;

class ColliderObject
{
public:
	ColliderObject(GameObject *object, int numberOfVertices = 2);
	~ColliderObject();

	ColliderType m_colliderType;
	GameObject *m_object;

	std::vector<ColliderVertex *> m_vertices;

	void DrawAndLabelContactPoints();
	void RemoveForces();

	void *m_userPointer;

	void CollisionDetectionUpdate(std::unordered_map<GameObject *, CollideeObject *> &collidees);

	std::vector<ColliderVertex *> GetVertexPositionsFor2DBox(const btVector3 &halfSize, int numberOfVertices);
	std::vector<btVector3> GetVertexVelocitiesFor2DBox(btRigidBody *body, const btVector3 &halfSize);
	std::vector<btVector3> GetForcesOnVertexes();
	void GetCollidingObjects(std::set<CollideeObject *> &collidingObjects);
	void GetCollidingGameObjects(std::set<GameObject *> &collidingObjects);

	float milliSecondsForReset = 2000;

private:
	void Initialize2DBox(int numberOfVertices);

};

