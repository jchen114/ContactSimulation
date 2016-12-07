#include "stdafx.h"
#include "ContactCollisionDispatcher.h"
#include "ContactManager.h"

ContactCollisionDispatcher::ContactCollisionDispatcher(btCollisionConfiguration * btCollisionConfiguration):btCollisionDispatcher(btCollisionConfiguration) {

}

ContactCollisionDispatcher::~ContactCollisionDispatcher()
{

}

#pragma region OVERRIDE

bool ContactCollisionDispatcher::needsCollision(btCollisionObject *body0, btCollisionObject *body1){

	if (ContactManager::GetInstance().m_forCollision.find((GameObject *)body0->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
		&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)body1->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
		// This object is about to collide..
		// obj0 is for collision
		// obj1 is to collide with
		return false;
	}
	else if (
		ContactManager::GetInstance().m_forCollision.find((GameObject *)body1->getUserPointer()) != ContactManager::GetInstance().m_forCollision.end()
		&& ContactManager::GetInstance().m_toCollideWith.find((GameObject *)body0->getUserPointer()) != ContactManager::GetInstance().m_toCollideWith.end()) {
		//printf("object 0 to collide object 1 for collision \n");
		return false;
	}
	else {
		return btCollisionDispatcher::needsCollision(body0, body1);
	}
}

#pragma endregion OVERRIDE
