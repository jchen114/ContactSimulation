#include "stdafx.h"
#include "ColliderObject.h"

#include "ColliderVertex.h"
#include "CollideeObject.h"


#pragma region INITIALIZE

ColliderObject::ColliderObject(GameObject *object, int numberOfVertices)
{
	m_object = object;
	m_userPointer = object;

	m_object->GetRigidBody()->setActivationState(DISABLE_DEACTIVATION);

	switch (m_object->GetRigidBody()->getCollisionShape()->getShapeType()) {
	case BOX_2D_SHAPE_PROXYTYPE:
	{
		Initialize2DBox(numberOfVertices);
	}
		break;
	case SPHERE_SHAPE_PROXYTYPE: {
		
	}
		break;
	default:
		break;
	}
}


ColliderObject::~ColliderObject()
{
}

void ColliderObject::Initialize2DBox(int numberOfVertices) {

	const btBox2dShape *box = static_cast<const btBox2dShape*>(m_object->GetRigidBody()->getCollisionShape());
	btVector3 halfSize = box->getHalfExtentsWithMargin();

	// Get vertices in a clockwise manner.
	m_vertices = GetVertexPositionsFor2DBox(halfSize, numberOfVertices);

}


void ColliderObject::CollisionDetectionUpdate(std::unordered_map<GameObject *, CollideeObject*> &collidees) {

	for (auto it = m_vertices.begin(); it != m_vertices.end(); it++) {
		ColliderVertex* vertex = *it;
		//printf("Collidees: %d\n", collidees.size());
		vertex->CollisionDetectionUpdate(collidees);
	}

}

#pragma endregion INITIALIZE

#pragma region INTERFACE

std::vector<ColliderVertex*> ColliderObject::GetVertexPositionsFor2DBox(const btVector3 &halfSize, int numberOfVertices) {
	
	std::vector<ColliderVertex*> vertex_positions = {
		new ColliderVertex(m_object, btVector3(-halfSize.x(), -halfSize.y(), 0), 0),
		new ColliderVertex(m_object, btVector3(-halfSize.x(), halfSize.y(), 0), 1),
		new ColliderVertex(m_object, btVector3(halfSize.x(), halfSize.y(), 0), 2),
		new ColliderVertex(m_object, btVector3(halfSize.x(), -halfSize.y(), 0), 3)
	};

	// For the bottom edge.
	float distanceBetweenVertices = halfSize.x() * 2 / (numberOfVertices - 2 + 1);
	for (int i = 1; i < numberOfVertices - 1; i++) {
		ColliderVertex *vert = new ColliderVertex(m_object, btVector3(-halfSize.x() + i * distanceBetweenVertices, -halfSize.y(), 0), 3 + i);
		vertex_positions.push_back(vert);
	}

	return vertex_positions;
}

std::vector<btVector3> ColliderObject::GetVertexVelocitiesFor2DBox(btRigidBody *body, const btVector3 &halfSize) {
	std::vector<btVector3> vertex_velocities = {
		body->getVelocityInLocalPoint(btVector3(-halfSize.x(), -halfSize.y(), 0)),
		body->getVelocityInLocalPoint(btVector3(-halfSize.x(), halfSize.y(), 0)),
		body->getVelocityInLocalPoint(btVector3(halfSize.x(), halfSize.y(), 0)),
		body->getVelocityInLocalPoint(btVector3(halfSize.x(), -halfSize.y(), 0))
	};
	return vertex_velocities;
}

std::vector<btVector3> ColliderObject::GetForcesOnVertexes() {

	std::vector<btVector3> r_forces;

	for (std::vector<ColliderVertex *>::iterator it = m_vertices.begin(); it != m_vertices.end(); ++it) {
		ColliderVertex *v = *it;
		btVector3 r_force = v->GetForce();
		r_forces.push_back(r_force);
	}

	return r_forces;

}

void ColliderObject::GetCollidingObjects(std::set<CollideeObject *> &collidingObjects) {

	for (auto vertex : m_vertices) {

		CollideeObject *collideeObject = vertex->GetCollidingObject();
		if (collideeObject) {
			collidingObjects.insert(collideeObject);
		}
		
	}

}

void ColliderObject::GetCollidingGameObjects(std::set<GameObject *> &collidingObjects) {

	for (auto vertex : m_vertices) {

		CollideeObject *collideeObject = vertex->GetCollidingObject();
		if (collideeObject) {
			collidingObjects.insert(collideeObject->m_object);
		}

	}

	//printf("collidingObjects size: %d\n", collidingObjects.size());

}

void ColliderObject::RemoveForces()	{

	for (auto vertex : m_vertices) {
		vertex->RemoveReactionForce();
	}

}

#pragma endregion INTERFACE

#pragma region DRAWING

void ColliderObject::DrawAndLabelContactPoints() {

	// Draw the forces present on the vertices
	for (auto it = m_vertices.begin(); it != m_vertices.end(); it++) {
		ColliderVertex *vertex = *it;
		vertex->DrawInfo();
		vertex->DrawForce();
	}

}

#pragma endregion DRAWING