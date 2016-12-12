#include "stdafx.h"
#include "CollideeObject.h"


CollideeObject::CollideeObject(GameObject *object, CollideeInterest interest, float friction_coeff, float ground_stiffness, float ground_damping)
{

	m_object = object;
	m_userPointer = object;

	m_interestType = interest;

	switch (object->GetRigidBody()->getCollisionShape()->getShapeType())
	{
	case BOX_2D_SHAPE_PROXYTYPE: {
		m_shapeType = COLLIDEE_BOX_2D_SHAPE;
	}
		break;
	case BOX_SHAPE_PROXYTYPE: {
		m_shapeType = COLLIDEE_BOX_SHAPE;
	}
		break;
	case SPHERE_SHAPE_PROXYTYPE:
	{
		m_shapeType = COLLIDEE_CIRCLE_SHAPE;
	}
		break;
	default:
		break;
	}

	m_friction = friction_coeff;
	m_ground_stiffness = ground_stiffness;
	m_ground_damping = ground_damping;

}

CollideeObject::~CollideeObject()
{
}

void CollideeObject::UpdateCollidingPlane() {
	switch (m_shapeType)
	{
	case COLLIDEE_BOX_2D_SHAPE: {
		
	}
		break;
	case COLLIDEE_BOX_SHAPE: {
		m_planes = HandleBoxCollidingPlanes();
	}
		break;
	default:
		break;
	}
}

btVector3 CollideeObject::GetCenter() {
	return m_object->GetCOMPosition();
}

float CollideeObject::GetRadius() {
	switch (m_shapeType)
	{
	case COLLIDEE_CIRCLE_SHAPE:
	{
		btSphereShape *myShape = static_cast<btSphereShape*> (m_object->GetShape());
		return myShape->getRadius();
	}
		break;
	default:
		break;
	}
	return 0.0f;
}

std::vector <std::pair<btVector3, btVector3>> CollideeObject::HandleBoxCollidingPlanes() {
	std::vector<std::pair<btVector3, btVector3>> planes;
	// Only handling the top plane right now..
	// TODO other planes, maybe
	btVector3 COM = m_object->GetCOMPosition();
	const btBoxShape *box = static_cast<const btBoxShape*>(m_object->GetRigidBody()->getCollisionShape());
	btVector3 halfSize = box->getHalfExtentsWithMargin();
	btVector3 vertex_1 = COM + btVector3(-halfSize.x(), halfSize.y(), -halfSize.z());
	btVector3 vertex_2 = COM + btVector3(halfSize.x(), halfSize.y(), halfSize.z());
	planes.push_back(std::pair<btVector3, btVector3>(vertex_1, vertex_2));

	return planes;
}

std::vector <std::pair<btVector3, btVector3>> CollideeObject::GetRelativePlanes() {

	std::vector<std::pair<btVector3, btVector3>> planes;
	// Only handling the top plane right now..
	const btBoxShape *box = static_cast<const btBoxShape*>(m_object->GetRigidBody()->getCollisionShape());
	btVector3 halfSize = box->getHalfExtentsWithMargin();
	btVector3 vertex_1 = btVector3(-halfSize.x(), halfSize.y(), -halfSize.z());
	btVector3 vertex_2 = btVector3(halfSize.x(), halfSize.y(), halfSize.z());
	planes.push_back(std::pair<btVector3, btVector3>(vertex_1, vertex_2));

	// Left Plane
	vertex_1 = btVector3(-halfSize.x(), -halfSize.y(), -halfSize.z());
	vertex_2 = btVector3(-halfSize.x(), halfSize.y(), halfSize.z());
	planes.push_back({ vertex_1, vertex_2 });

	// Right Plane
	vertex_1 = btVector3(halfSize.x(), -halfSize.y(), -halfSize.z());
	vertex_2 = btVector3(halfSize.x(), halfSize.y(), halfSize.z());
	planes.push_back({ vertex_1, vertex_2 });

	return planes;

}

std::vector <std::pair<btVector3, btVector3>> CollideeObject::Handle2DBoxCollidingPlanes() {
	std::vector<std::pair<btVector3, btVector3>> planes;
	btVector3 COM = m_object->GetCOMPosition();
	// top
	const btBox2dShape *box = static_cast<const btBox2dShape *>(m_object->GetRigidBody()->getCollisionShape());
	btVector3 halfSize = box->getHalfExtentsWithMargin();
	btVector3 vertex_1 = COM + btVector3(-halfSize.x(), halfSize.y(), 0);
	btVector3 vertex_2 = COM + btVector3(halfSize.x(), halfSize.y(), 0);
	planes.push_back(std::pair<btVector3, btVector3>(vertex_1, vertex_2));

	return planes;
}

std::vector <std::pair<btVector3, btVector3>> CollideeObject::GetPlanes() {
	return m_planes;
}

float CollideeObject::GetFriction() {
	return m_friction;
}

std::tuple<float, float> CollideeObject::GetGroundProperties() {

	return std::make_tuple(m_ground_stiffness, m_ground_damping);

}