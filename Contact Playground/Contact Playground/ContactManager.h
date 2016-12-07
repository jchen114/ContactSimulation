#pragma once

#include "GameObject.h"
#include <unordered_set>
#include "BulletCollision\CollisionShapes\btBox2dShape.h"
#include "BulletOpenGLApplication.h"

#include "ColliderObject.h"
#include "CollideeObject.h"

#include <unordered_map>

enum CollisionTypes {
	COL_CONTACT_MODEL = 128
};

class ContactManager
{

public:

	static ContactManager& GetInstance() {
		static ContactManager instance;
		return instance;
	}

	ContactManager();
	~ContactManager();

	ColliderObject *AddObjectForCollision(GameObject *object, int numberOfVertices = 2);
	CollideeObject *AddObjectToCollideWith(GameObject *object, float friction = 0.7f);

	int RemoveObjectForCollision(GameObject *object);
	int RemoveObjectToCollideWith(GameObject *object);

	void ClearObjectsToCollideWith();

	btOverlapFilterCallback *GetFilterCallback();

	void DrawContactPoints();

	std::unordered_map<GameObject *, ColliderObject> m_forCollision;
	std::unordered_map<GameObject *, CollideeObject> m_toCollideWith;

	void Update(btScalar timestep);

	void AddCollisionPair(ColliderObject *obj_collider, CollideeObject *obj_collidee);
	void RemoveCollisionPair(GameObject *obj);

	void GetCollideesForObject(ColliderObject *objQ, std::set<CollideeObject *> &collidingObjects);
	void GetCollidingGameObjectsForObject(ColliderObject *objQ, std::set<GameObject *> &gameObjects);

	std::unordered_map<GameObject *, GameObject *> m_collisionPairs;

	bool m_beingUsed = false;

	static void MyNearCallback(
		btBroadphasePair& collisionPair,
		btCollisionDispatcher& dispatcher,
		const btDispatcherInfo& dispatchInfo
		) {

		btBroadphaseProxy *proxy0 = collisionPair.m_pProxy0;
		btBroadphaseProxy *proxy1 = collisionPair.m_pProxy1;

		btCollisionObject *obj0 = (btCollisionObject *)proxy0->m_clientObject;
		btCollisionObject *obj1 = (btCollisionObject *)proxy1->m_clientObject;

		bool toContinue = true;

		if (ContactManager::GetInstance().m_forCollision.find((GameObject *)obj0->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
			&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)obj1->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
			// This object is about to collide..
			// obj0 is for collision
			// obj1 is to collide with

			// Add this pair to pairs to check
			ContactManager::GetInstance().AddCollisionPair((GameObject *)ContactManager::GetInstance().m_forCollision.find(obj0->getUserPointer()), (GameObject *)m_toCollideWith.find(obj1.getUserPointer()));
			toContinue = false;
		}
		else if (
			ContactManager::GetInstance().m_forCollision.find((GameObject *)obj1->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
			&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)obj0->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
			//printf("object 0 to collide object 1 for collision \n");

			// Add this pair to pairs to check

			toContinue = false;
		}

		if (toContinue) {
			// Do your collision logic here
			// Only dispatch the Bullet collision information if you want the physics to continue
			dispatcher.defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
		}

	}

private:

	btOverlapFilterCallback *m_filterCallback;

};

struct ContactFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
	{
		btCollisionObject *obj0 = (btCollisionObject *)proxy0->m_clientObject;
		btCollisionObject *obj1 = (btCollisionObject *)proxy1->m_clientObject;
		if (ContactManager::GetInstance().m_forCollision.find((GameObject *)obj0->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
			&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)obj1->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
			// This object is about to collide..
			// obj0 is for collision
			// obj1 is to collide with
			return false;
		}
		else if (
			ContactManager::GetInstance().m_forCollision.find((GameObject *)obj1->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
			&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)obj0->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
			//printf("object 0 to collide object 1 for collision \n");
			return false;
		}
		else {
			return true;
		}
		

	}
};

