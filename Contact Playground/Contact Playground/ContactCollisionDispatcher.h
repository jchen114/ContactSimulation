#pragma once

#include "BulletCollision\CollisionDispatch\btCollisionDispatcher.h"

class ContactCollisionDispatcher : public btCollisionDispatcher
{
public:

	ContactCollisionDispatcher::ContactCollisionDispatcher(btCollisionConfiguration * btCollisionConfiguration);

	~ContactCollisionDispatcher();

	// Override
	virtual bool needsCollision(btCollisionObject *body0, btCollisionObject *body1);
};

