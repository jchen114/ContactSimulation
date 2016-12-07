#include "stdafx.h"
#include "ContactManager.h"


#pragma region INITIALIZE

ContactManager::ContactManager()
{
}

ContactManager::~ContactManager()
{
}

btOverlapFilterCallback *ContactManager::GetFilterCallback() {
	static bool initialized;
	if (!initialized) {
		m_filterCallback = new ContactFilterCallback();
		initialized = true;
	}
	return m_filterCallback;
}

#pragma endregion INITIALIZE

#pragma region CONTACT_INTERFACE

void ContactManager::ClearObjectsToCollideWith() {
	m_toCollideWith.clear();
}

ColliderObject *ContactManager::AddObjectForCollision(GameObject *object, int numberOfVertices) {

	m_beingUsed = true;

	ColliderObject *colliderObject = new ColliderObject(object, numberOfVertices);
	m_forCollision.insert({ object, *colliderObject });
	return colliderObject;
}

CollideeObject *ContactManager::AddObjectToCollideWith(GameObject *object, float friction){
	CollideeObject *collideeObject = new CollideeObject(object, OBJECT_OF_INTEREST, friction);
	m_toCollideWith.insert({ object, *collideeObject });
	printf("Add object to collide with size: %d\n", m_toCollideWith.size());
	return collideeObject;
}

int ContactManager::RemoveObjectForCollision(GameObject *object) {
	// TODO
	m_forCollision.erase(object);
	return 0;
}

int ContactManager::RemoveObjectToCollideWith(GameObject *object) {
	// TODO
	return 0;
}

void ContactManager::GetCollideesForObject(ColliderObject *objQ, std::set<CollideeObject *> &collidingObjects) {
	objQ->GetCollidingObjects(collidingObjects);
}

void ContactManager::GetCollidingGameObjectsForObject(ColliderObject *objQ, std::set<GameObject *> &gameObjects) {
	objQ->GetCollidingGameObjects(gameObjects);
}


#pragma endregion CONTACT_INTERFACE

#pragma region DRAWING

void ContactManager::DrawContactPoints() {
	for (auto it = m_forCollision.begin(); it != m_forCollision.end(); it++) {
		ColliderObject colliderObject = it->second;
		colliderObject.DrawAndLabelContactPoints();
	}
}

#pragma endregion DRAWING

void ContactManager::Update(btScalar timestep) {
	std::unordered_map<GameObject *, CollideeObject>::iterator kv;
	for (kv = m_toCollideWith.begin(); kv != m_toCollideWith.end(); kv ++) {
		CollideeObject& obj = kv->second;
		obj.UpdateCollidingPlane();
	}

	for (auto it = m_forCollision.begin(); it != m_forCollision.end(); it++) {
		ColliderObject& colliderObject = it->second;
		colliderObject.CollisionDetectionUpdate(m_toCollideWith);
	}

}

void ContactManager::AddCollisionPair(GameObject *obj1, GameObject *obj2) {

	m_collisionPairs.insert({ obj1, obj2 });

}

void ContactManager::RemoveCollisionPair(GameObject *obj) {

	m_collisionPairs.erase(obj);

}