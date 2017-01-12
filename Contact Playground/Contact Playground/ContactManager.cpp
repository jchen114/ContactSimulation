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

CollideeObject *ContactManager::AddGroundToCollideWith(GameObject *object, float ground_stiffness, float ground_damping, float friction) {
	CollideeObject *collideeObject = new CollideeObject(object, OBJECT_OF_INTEREST, friction, ground_stiffness, ground_damping);
	m_toCollideWith.insert({ object, *collideeObject });
	printf("Add Ground to collide with size: %d\n", m_toCollideWith.size());
	return collideeObject;
}

int ContactManager::RemoveObjectForCollision(GameObject *object) {
	// TODO
	m_forCollision.erase(object);
	return 0;
}

int ContactManager::RemoveObjectToCollideWith(GameObject *object) {
	// TODO
	m_toCollideWith.erase(object);
	printf("Remove object to collide with size: %d\n", m_toCollideWith.size());
	return 0;
}

void ContactManager::GetCollideesForObject(ColliderObject *objQ, std::set<CollideeObject *> &collidingObjects) {
	objQ->GetCollidingObjects(collidingObjects);
}

void ContactManager::GetCollidingGameObjectsForObject(ColliderObject *objQ, std::set<GameObject *> &gameObjects) {
	objQ->GetCollidingGameObjects(gameObjects);

	/*if (gameObjects.empty()) {
		RemoveCollisionPair(objQ->m_object);
	}*/

}

void ContactManager::Reset() {

	// Remove all the forces that are felt by the colliders
	std::unordered_map<GameObject *, ColliderObject>::const_iterator iter;
	for (iter = m_forCollision.begin(); iter != m_forCollision.end(); iter++) {
		ColliderObject collider = iter->second;
		collider.RemoveForces();
	}

	m_collisionPairs.clear();
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

		auto collisionPairIter = m_collisionPairs.find(colliderObject.m_object);
		if (collisionPairIter != m_collisionPairs.end()) {
			std::set<GameObject*> collidees = collisionPairIter->second;

			std::unordered_map<GameObject *, CollideeObject *> c_map;

			for (auto it : collidees) {
				CollideeObject *cObj = &m_toCollideWith.find(it)->second;
				c_map.insert({ it, cObj });
			}

			colliderObject.CollisionDetectionUpdate(c_map);
		}
		
	}

}

void ContactManager::AddCollisionPair(GameObject * obj_collider, GameObject * obj_collidee) {

	auto iter = m_collisionPairs.find(obj_collider);

	if (iter != m_collisionPairs.end()) {
		iter->second.insert(obj_collidee);
	}
	else {
		m_collisionPairs.insert({ obj_collider, std::set<GameObject*> { obj_collidee } });
	}

}

void ContactManager::RemoveCollisionPair(GameObject * obj_collider, GameObject *obj_collidee) {

	auto iter = m_collisionPairs.find(obj_collider);

	if (iter != m_collisionPairs.end()) {
		iter->second.erase(obj_collidee);
	}

}