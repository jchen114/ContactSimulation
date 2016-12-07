#include "stdafx.h"
#include "LearningManager.h"

#include "Sample.pb.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/functional/hash.hpp>

#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <random>

#include "sqlite3.h"

namespace fs = boost::filesystem;

const std::string net_prototxt_path = "..\\..\\Dependencies\\Caffe\\QLearningNet.prototxt";
const std::string solver_prototxt_path = "..\\..\\Dependencies\\Caffe\\QSolver.prototxt";

const std::string db_path = "..\\..\\Dependencies\\Caffe\\train_db\\samples.db";
const std::string size_file_path = "..\..\Dependencies\\Caffe\\train_db\\size.txt";

const std::string param_path = "..\\..\\Dependencies\\Caffe\\Params";

DEFINE_string(backend, "lmdb", "The backend {lmdb, leveldb} for storing the result");

static LearningManager *m_me;

LearningManager::LearningManager()
{
	int rc = sqlite3_open(db_path.c_str(), &samples_db);
	if (rc) {
		printf("Can't open database. \n");
	}
	else {
		printf("Database successfully opened. \n");
	}

	char *sql_stmt;
	char *zErrorMsg;
	sql_stmt = "CREATE TABLE IF NOT EXISTS SAMPLE("	\
		"ID INTEGER PRIMARY KEY NOT NULL," \
		"STATE REAL NOT NULL," \
		"ACTION INTGER NOT NULL, " \
		"REWARD REAL NOT NULL, " \
		"NEXT_STATE TEXT NOT NULL);";

	rc = sqlite3_exec(samples_db, sql_stmt, NULL, 0, &zErrorMsg);
	if (rc != SQLITE_OK) {
		printf("Sql error: %s \n", zErrorMsg);
	}
	else {
		printf("Table created successfully\n");
	}

	m_numberOfSamplesInDb = 0;
	LoadNet();
	LoadSolver();
	LoadParams();
	
	m_me = this;
	CountSamples();
}

LearningManager::~LearningManager()
{
	sqlite3_close(samples_db);
	delete samples_db;
}

#pragma region INTERFACE

Action LearningManager::QueryForAction(State queryState) {
	
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> dis(0, 1);
	
	float eps_prob = GetEpsilonProbability();
	printf("Epsilon probability = %f ", eps_prob);
	AgentAction a;
	if (dis(gen) <= eps_prob) {
		// Random action
		printf("Choose random Action. ");
		std::uniform_int_distribution<> distr(0, 6);
		a = (AgentAction) distr(gen);
		printf("Choose action %d \n", a);
		
	}
	else {
		printf("Choose Action from policy. \n");
		std::vector<tData> q_values = ForwardPassNetwork(queryState);
		int ac = 0;
		for (int i = 1; i < q_values.size(); i++) {
			if (q_values[i] > q_values[ac]) {
				ac = i;
			}
		}
		a = (AgentAction)ac;
	}

	printf("Choosing action %d\n", a);

	return Action((AgentAction)a);

}

void LearningManager::AddSample(SARS s) {

	// Insert sample into the database
	char *zErrMsg = 0;
	int rc;
	const char *sql_cmd = "INSERT INTO SAMPLE(ID, STATE, ACTION, REWARD, NEXT_STATE) VALUES(NULL, ?, ?, ?, ?)";
	sqlite3_stmt *pStmt;
	rc = sqlite3_prepare(samples_db, sql_cmd, -1, &pStmt, 0);
	if (rc != SQLITE_OK) {
		printf("SQL error: %s\n", zErrMsg);
	}
	else {
		printf("Preparation ok.\n");
	}
	// Bind state
	// For States with continous sensor data
	//std::string state_str;
	//s.SerializeCurrStateToString(state_str);
	//sqlite3_bind_text(pStmt, 1, state_str.c_str(), state_str.size(), SQLITE_STATIC);
	sqlite3_bind_double(pStmt, 1, s.GetCurrentState().GetRelativePosition());
	sqlite3_bind_int(pStmt, 2, s.GetAction());
	sqlite3_bind_double(pStmt, 3, s.GetRewardValue());
	
	//std::string s_n_str;
	//s.SerializeNextStateToString(s_n_str);
	//sqlite3_bind_text(pStmt, 4, s_n_str.c_str(), s_n_str.size(), SQLITE_STATIC);
	sqlite3_bind_double(pStmt, 4, s.GetNextState().GetRelativePosition());
	rc = sqlite3_step(pStmt);
	if (rc != 101) {
		printf("response: %s\n", sqlite3_errstr(rc));
	} 
	sqlite3_finalize(pStmt);
	m_numberOfSamplesInDb++;
	TrainSamples();
}

void LearningManager::CountSamples() {
	// Read some random values from the database.
	const char *sql_cmd = "SELECT COUNT (*) FROM SAMPLE";
	char *zErrMsg = 0;
	int rc = sqlite3_exec(samples_db, sql_cmd, CountCallback, (void *)0, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("Error msg: %s \n", zErrMsg);
	}
}

void LearningManager::TrainSamples() {

	if (m_numberOfSamplesInDb > BATCH_SIZE) { // Train for every step.
		SampleSamples();
	}
}

void LearningManager::AddTrainingSample(SARS sars) {
	m_sars.push_back(sars);
	if (m_sars.size() == BATCH_SIZE) {
		TrainNetwork();
	}
}

void LearningManager::SampleSamples() {
	std::srand(unsigned(std::time(0)));
	std::vector<int> idx;
	char *zErrMsg = 0;
	// set some values:
	for (int i = 1; i<=m_numberOfSamplesInDb; ++i) idx.push_back(i);

	std::random_shuffle(idx.begin(), idx.end());

	std::stringstream sql_stmt;
	sql_stmt << "SELECT * FROM SAMPLE WHERE ID IN (";

	for (int i = 0; i < BATCH_SIZE; i++) {
		sql_stmt << idx[i];
		if (i != BATCH_SIZE - 1) {
			sql_stmt << ",";
		}
	}
	
	sql_stmt << ")";

	int rc = sqlite3_exec(samples_db, sql_stmt.str().c_str(), SamplesCallback, (void *)0, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("Error msg: %s \n", zErrMsg);
	}

}

float LearningManager::GetEpsilonProbability() {

	return EPSILON * (1 / (1 + ((float)m_numberOfSamplesInDb / ANNEALING_FACTOR)));

}

#pragma SQL_CALLBACKS

static int CountCallback(void *data, int argc, char **argv, char **azColName) {

	for (int i = 0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		m_me->m_numberOfSamplesInDb = std::stoi(argv[i]);
	}
	printf("\n");

	return 0;
}

static int SamplesCallback(void *data, int argc, char **argv, char **azColName) {

	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	SARS sars;
	State s;
	Action a;
	Reward r;
	State s_n;
	for (i = 0; i<argc; i++){
		std::string col = azColName[i];
		std::string val = argv[i];
		
		if (!col.compare("STATE")) {
			s = State(std::stof(val));
		}
		if (!col.compare("ACTION")) {
			a = Action((AgentAction)std::stoi(val));
		}
		if (!col.compare("REWARD")) {
			r = Reward(std::stof(val));
		}
		if (!col.compare("NEXT_STATE")) {
			s_n = State(std::stof(val));
		}
	}
	sars = SARS(s, a, r, s_n);
	m_me->AddTrainingSample(sars);
	return 0;

}

#pragma endregion SQL_CALLBACKS

#pragma endregion INTERFACE

#pragma region CAFFE

void LearningManager::LoadNet () {
	m_net = std::unique_ptr<caffe::Net<tData>>(new caffe::Net<tData>(net_prototxt_path, caffe::TEST));
}

void LearningManager::LoadSolver() {
	caffe::SolverParameter solver_param;
	caffe::ReadProtoFromTextFileOrDie(solver_prototxt_path, &solver_param);
	caffe::Caffe::set_mode(caffe::Caffe::CPU);
	m_solver = std::unique_ptr<caffe::Solver<tData>>(caffe::SolverRegistry<tData>::CreateSolver(solver_param));
}

std::vector<tData> LearningManager::ForwardPassNetwork(State q) {

	//std::vector<tData> collapsed_state;
	//SquashState(q, collapsed_state);

	const std::vector<caffe::Blob<tData>*> &input_blobs = m_net->input_blobs();
	const auto& input_blob = input_blobs[0];
	tData *input_data = input_blob->mutable_cpu_data();
	for (int i = 0; i < input_blob->count(); i++) {
		input_data[i] = q.GetRelativePosition(); // Relative position of bump to feeler
	}

	const std::vector<caffe::Blob<tData> *> &result_arr = m_net->Forward();
	const caffe::Blob<tData>* result = result_arr[result_arr.size() - 1];
	const tData* result_data = result->cpu_data();
	std::vector <float> q_values;
	q_values.reserve(result->count());
	printf("Result: ");
	for (int i = 0; i < result->count(); i++) {
		printf(" %f ", result_data[i]);
		q_values.push_back(result_data[i]);
	}
	printf("\n");
	return q_values;
}

void LearningManager::TrainNetwork() {
	// Forward pass samples through network to get labels.
	std::vector<tData> batch_inputs;
	std::vector<tData> batch_labels;

	for (int i = 0; i < m_sars.size(); i++) {
		SARS sars = m_sars[i];
		State s = sars.GetCurrentState();
		// Forward pass of network for S to get the old value
		std::vector<tData> Q_values = ForwardPassNetwork(s);
		State s_n = sars.GetNextState();
		// Forward pass of network for S'
		std::vector<tData> Q_value_n = ForwardPassNetwork(s_n);
		// Get the max action value at S'
		std::vector<tData>::iterator it = std::max_element(Q_value_n.begin(), Q_value_n.end());
		// Create labels for training
		std::vector<tData> labels = Q_values;
		int actionIdx = sars.GetAction(); // Gets the action index taken
		labels[actionIdx] = sars.GetRewardValue() + DISCOUNT * (*it); // Update the action.
		batch_labels.insert(batch_labels.begin(), labels.begin(), labels.end());
		batch_inputs.push_back(s.GetRelativePosition());
	}

	// Solve the network
	auto train_net = m_solver->net();
	
	// Shove the data into the network
	const std::vector<caffe::Blob<tData>*> &input_blobs = train_net->input_blobs();
	const auto& input_blob0 = input_blobs[0];
	tData *input_data0 = input_blob0->mutable_cpu_data();
	for (int i = 0; i < input_blob0->count(); i++) {
		input_data0[i] = batch_inputs[i]; // Relative position of bump to feeler
	}
	const auto& input_blob1 = input_blobs[1];
	tData* input_data1 = input_blob0->mutable_cpu_data();
	for (int i = 0; i < input_blob0->count(); i++) {
		input_data1[i] = batch_labels[i]; // labels
	}
	m_solver->Solve();
	m_sars.clear();

	// Move parameters from Solver to forward network.
	caffe::NetParameter params;
	train_net->ToProto(&params);
	m_net->CopyTrainedLayersFrom(params);

	std::fstream pFile(param_path, std::ios::out | std::ios::binary);
	params.SerializePartialToOstream(&pFile);

}

void LearningManager::SquashState(State q, std::vector<tData> &vec) {

	std::vector<std::tuple<float, float>> state = q.GetContactState();

	std::for_each(state.begin(), state.end(), [&vec](std::tuple<float, float> force) mutable {
		vec.push_back(std::get<0>(force));
		vec.push_back(std::get<1>(force));
	});

}

void LearningManager::LoadParams() {
	if (boost::filesystem::exists(param_path)) {

		printf("Param file found. Load params\n");

		caffe::NetParameter params;
		std::fstream pFile(param_path, std::ios::in | std::ios::binary);
		params.ParseFromIstream(&pFile);
		// Set train net and forward net
		m_net->CopyTrainedLayersFrom(params);
		m_solver->net()->CopyTrainedLayersFrom(params);
	}
}

#pragma endregion CAFFE