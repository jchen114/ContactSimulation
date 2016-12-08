#pragma once

#include "LinearMath\btVector3.h"
#include "CollideeObject.h"
#include "Constants.h"
#include "ContactLearningApp.h"
#include <unordered_map>
#include <memory>

typedef enum {
	IN_COLLISION = 0,
	NO_COLLISION
} VertexState;

typedef enum {
	TOP_PLANE = 0,
	LEFT_PLANE,
	RIGHT_PLANE
}VertexLocation;

class ColliderVertex
{
public:
	ColliderVertex(GameObject *object, const btVector3 &offset, int vid);
	~ColliderVertex();

	void CollisionDetectionUpdate(std::unordered_map<GameObject *, CollideeObject *> &objects);

	btScalar m_springConstant = 2000.0f;
	btScalar m_dampingConstant = 200.0f;
	btScalar m_friction;
	int m_id;

	void DrawForce();
	void DrawInfo();

	void RemoveReactionForce();

	btVector3 GetForce();
	CollideeObject *GetCollidingObject();
	VertexState GetState();

private:

	//std::unique_ptr<btPairCachingGhostObject> m_ghostVertex;
	//std::unique_ptr<btSphereShape> sphere;

	void CheckForCollision(std::unordered_map<GameObject *, CollideeObject *> &objects);
	void ManageCollision(std::unordered_map<GameObject *, CollideeObject *> &objects);

	CollideeShapeType m_shapeInCollision;

	VertexLocation m_location;
	btVector3 m_vertexPos;
	btVector3 m_vertexVel;
	VertexState m_state = NO_COLLISION;
	btVector3 m_previousPoint;
	btVector3 m_collisionPoint;
	GameObject *m_object;
	btVector3 m_offset;
	btVector3 m_newOffset;
	btScalar m_distanceFromCOM;
	btScalar m_orientation;

	GameObject *m_contactObject;
	CollideeObject *m_collideeObject;

	btVector3 m_springForce = btVector3(0.0f, 0.0f, 0.0f);
	btVector3 m_dampingForce = btVector3(0.0f, 0.0f, 0.0f);

	btVector3 m_reactionForce = btVector3(0.0f, 0.0f, 0.0f);

	float m_minAngle;
	float m_maxAngle;

	void HandleBoxCollision(std::vector<std::pair<btVector3, btVector3>> planes);
	void HandleBoxCollision(CollideeObject* cObj);
	void Handle2DBoxCollision(std::vector<std::pair<btVector3, btVector3>> planes);
	void HandleCircleCollision(const btVector3 &center, float radius);

	void HandleTopPlaneBoxCollision(std::pair<btVector3, btVector3> topPlane);

};

float AngleToRotateForCollision(float angle, const btVector3 &vertexVector, float radius);