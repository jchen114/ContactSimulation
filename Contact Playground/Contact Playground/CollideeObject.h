#pragma once

#include "GameObject.h"
#include "BulletCollision\CollisionShapes\btBox2dShape.h"
#include <tuple>

typedef enum {
	COLLIDEE_BOX_2D_SHAPE,
	COLLIDEE_BOX_SHAPE,
	COLLIDEE_CIRCLE_SHAPE
} CollideeShapeType;

typedef enum {
	OBJECT_OF_INTEREST,
	OBJECT_NOT_OF_INTEREST
}CollideeInterest;

class CollideeObject
{
public:
	CollideeObject(
		GameObject *object, 
		CollideeInterest interest = OBJECT_NOT_OF_INTEREST, 
		float friction_coeff=2.0f, 
		float ground_stiffness=2500.0f, 
		float ground_damping=250.0f);

	~CollideeObject();

	void *m_userPointer;
	CollideeShapeType m_shapeType;
	GameObject *m_object;

	void UpdateCollidingPlane();
	std::vector <std::pair<btVector3, btVector3>> GetPlanes();
	std::vector <std::pair<btVector3, btVector3>> GetRelativePlanes();
	btVector3 GetCenter();

	float GetRadius();
	float GetFriction();

	std::tuple<float, float> GetGroundProperties();

	CollideeInterest m_interestType;

private:

	std::vector <std::pair<btVector3, btVector3>> HandleBoxCollidingPlanes();
	std::vector <std::pair<btVector3, btVector3>> Handle2DBoxCollidingPlanes();
	std::vector <std::pair<btVector3, btVector3>> m_planes;

	float m_friction;
	float m_ground_stiffness;
	float m_ground_damping;

};

