#pragma once

#include "GameObject.h"
#include <vector>
#include <random>

class TerrainCreator
{
public:
	TerrainCreator(
		float mean_length = 2.0f,
		float std_dev = 0.01f,
		float height = 0.5f,
		float depth = 3,
		float slope_min = 0.13f,
		float slope_max = 0.13f,
		float friction_min = 0.1f, 
		float friction_max = 2.0f
		);
	~TerrainCreator();

	std::vector<GameObject *> CreateTerrains(const btVector3 &startPoint);

private:
	float m_mean_length;
	float m_std_dev;
	float m_slope_min;
	float m_slope_max;
	float m_fric_min;
	float m_fric_max;
	float m_height;
	float m_depth;

	std::default_random_engine m_generator;
	std::normal_distribution<double> m_distribution;

	
	std::mt19937 m_unigen;
	std::uniform_real_distribution<> m_dis;

};

