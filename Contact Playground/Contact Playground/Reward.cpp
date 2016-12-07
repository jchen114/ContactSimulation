#include "stdafx.h"
#include <math.h>

#include "Reward.h"

#include "ColliderObject.h"
#include "CollideeObject.h"

Reward::Reward(std::unique_ptr<ColliderObject> &feelerObj, std::set<CollideeObject*> &collisionObjs, float lowerBound)
{
	// Calculate the reward based on the state
	float max_reward = -INFINITY;
	btVector3 feelerCOM = feelerObj->m_object->GetCOMPosition();

	btVector3 min, max;
	feelerObj->m_object->GetRigidBody()->getAabb(min, max);
	float width = max.x() - min.x();
	//printf("Min: (%f, %f, %f), Max: (%f, %f, %f)\n");

	for (auto obj : collisionObjs) {
		switch (obj->m_shapeType)
		{
		case COLLIDEE_CIRCLE_SHAPE:
		{
			// query is between -width to width
			btVector3 objCOM = obj->m_object->GetCOMPosition();
			float query = feelerCOM.x() - objCOM.x();
			float reward = RewardFunc(query, width/2, lowerBound);
			if (max_reward < reward) {
				max_reward = reward;
				m_distance = query;
			}
		}
			break;
		case COLLIDEE_BOX_SHAPE:
		{
			max_reward < lowerBound ? max_reward = lowerBound : max_reward = max_reward;
		}
			break;
		default:
			break;
		}
	}
	
	if (max_reward < lowerBound) {
		printf("ITS LESS THAN -9000!!! \n");
	}

	printf("Reward: %f\n", max_reward);
	m_reward = max_reward;
}

Reward::Reward(float reward) {
	m_reward = reward;
}

Reward::Reward() 
{

}

Reward::~Reward()
{

}

float Reward::GetReward() {

	return m_reward;

}

float Reward::GetDistToCollidingObject() {
	return m_distance;
}


float Reward::RewardFunc(float query, float width, float lowerBound) {
	
	if (query < -width || query > width) {
		return lowerBound;
	}

	return (lowerBound / powf(width, 2) * powf(query, 2));

}