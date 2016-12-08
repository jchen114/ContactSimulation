#include "stdafx.h"
#include "TerrainCreator.h"


TerrainCreator::TerrainCreator(float mean_length, float std_dev, float height, float depth, float slope_min, float slope_max, float friction_min, float friction_max)
{
	m_mean_length = mean_length;
	m_std_dev = std_dev;
	m_slope_min = slope_min;
	m_slope_max = slope_max;
	m_fric_min = friction_min;
	m_fric_max = friction_max;
	m_height = height;
	m_depth = depth; 

	m_distribution = std::normal_distribution<double>(m_mean_length, m_std_dev);

	std::random_device rd;
	m_unigen = std::mt19937(rd());
	std::uniform_real_distribution<> m_dis(m_slope_min, m_slope_max);
}


TerrainCreator::~TerrainCreator()
{

}


std::vector<GameObject *> TerrainCreator::CreateTerrains(const btVector3 &startPoint) {

	std::vector<GameObject*> slabs;

	// Sample Slope.
	float num = m_dis(m_unigen); // from 0 - 1
	float slope = num * (m_slope_max - m_slope_min) + m_slope_min;
	printf("slope = %f\n", slope);
	float rotation_angle = atan(slope);

	// First slab..
	double length = m_distribution(m_generator);
	GameObject *slab0 = new GameObject(new btBoxShape(btVector3(length, m_height, m_depth)), 0.0f, btVector3(0.0f, 1.0f, 0.0f), startPoint + btVector3(length, 0, 0), "Ground Up");

	btTransform tr;
	// Rising plane
	slab0->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	tr.setRotation(btQuaternion(btVector3(0,0,1), rotation_angle));

	float x_to_move = length - length * cos(rotation_angle);

	tr.setOrigin(slab0->GetCOMPosition() + btVector3(-x_to_move, length * sin(rotation_angle), 0));
	//tr.setOrigin(slab0->GetCOMPosition() + btVector3(-0, 10, 0));
	slab0->GetRigidBody()->getMotionState()->setWorldTransform(tr);
	slab0->GetRigidBody()->setWorldTransform(tr);
	slabs.push_back(slab0);

	slab0->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	btVector3 endpt = tr(btVector3(length, 0, 0));
	// Flat plane
	GameObject *slab1 = new GameObject(new btBoxShape(btVector3(length, m_height, m_depth)), 0.0f, btVector3(0.0f, 0.0f, 1.0f), endpt + btVector3(length, 0, 0), "Ground Flat");
	slabs.push_back(slab1);

	slab1->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	endpt = tr(btVector3(length, 0, 0));
	// Falling Plane
	GameObject *slab2 = new GameObject(new btBoxShape(btVector3(length, m_height, m_depth)), 0.0f, btVector3(1.0f, 0.0f, 1.0f), endpt + btVector3(length, 0, 0), "Ground Down");

	slab2->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	tr.setRotation(btQuaternion(btVector3(0, 0, 1), -rotation_angle));
	tr.setOrigin(slab2->GetCOMPosition() + btVector3(-x_to_move, - length * sin(rotation_angle), 0));
	slab2->GetRigidBody()->getMotionState()->setWorldTransform(tr);
	slab2->GetRigidBody()->setWorldTransform(tr);
	slabs.push_back(slab2);

	// Flat plane
	slab2->GetRigidBody()->getMotionState()->getWorldTransform(tr);
	endpt = tr(btVector3(length, 0, 0));

	GameObject *slab3 = new GameObject(new btBoxShape(btVector3(length, m_height, m_depth)), 0.0f, btVector3(0.0f, 1.0f, 1.0f), endpt + btVector3(length, 0, 0));

	slabs.push_back(slab3);

	return slabs;
}