#pragma once

#include "Action.h"
#include "State.h"
#include "SARS.h"

#define GLOG_NO_ABBREVIATED_SEVERITIES

#include <caffe\caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>

#include <utility>
#include <vector>

#include <memory>
#include <tuple>

class sqlite3;

typedef float tData;

#define BATCH_SIZE 32
#define DISCOUNT 0.9f
#define EPSILON 0.4f
#define ANNEALING_FACTOR 100.0f
#define SAMPLE_SIZE 1

class LearningManager
{
public:
	LearningManager();
	~LearningManager();

	Action QueryForAction(State queryState);
	void AddSample(SARS s);

	int m_numberOfSamplesInDb;
	std::vector<tData> ForwardPassNetwork(State q);

	void TrainSamples();
	void SampleSamples();
	void CountSamples();
	void TrainNetwork();

	void AddTrainingSample(SARS sars);

private:
	std::unique_ptr <caffe::Net<tData>> m_net;
	std::unique_ptr <caffe::Solver<tData>> m_solver;

	sqlite3 *samples_db;

	std::vector<SARS> m_sars;

	long sizeOfDatabase;

	void LoadNet();
	void LoadSolver();
	void LoadParams();
	void SquashState(State q, std::vector<tData> &vec);

	float GetEpsilonProbability();

};

static int CountCallback(void *data, int argc, char **argv, char **azColName);
static int SamplesCallback(void *data, int argc, char **argv, char **azColName);