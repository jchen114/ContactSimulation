#pragma once

#include "LinearMath\btVector3.h"
#include <set>
#include <memory>

class ColliderObject;
class CollideeObject;

class Reward
{
public:
	Reward(std::unique_ptr<ColliderObject> &feelerObj, std::set<CollideeObject *> &collisionObjs, float lowerBound);
	Reward(float reward);
	Reward();
	~Reward();

	float GetReward();
	float GetDistToCollidingObject();


private:
	float m_reward;
	float m_distance;
	float RewardFunc(float query, float width, float lowerBound);

};

