import sqlite3
import copy
import sys
maj = sys.version_info

version = 2

if maj[0] >= 3:
	import _pickle as pickle
	import importlib.machinery
	import types
	version = 3
else:
	import cPickle as pickle
	import imp
import math
import numpy as np
from random import shuffle

def initialize_db_connection(db_file):
	conn = sqlite3.connect(db_file, check_same_thread=False)
	return conn


def get_number_of_sequences(db_conn):
	cursor = db_conn.cursor()
	cursor.execute("SELECT MAX(SEQUENCE_ID) FROM SEQUENCES")
	num_sequences = cursor.fetchone()
	return num_sequences


def get_sequence(db_conn, sequence_id):
	cursor = db_conn.cursor()
	cursor.execute("SELECT STATE_ID FROM SEQUENCES WHERE SEQUENCE_ID = {SID} ORDER BY SEQ_ORDER ASC".format(SID=sequence_id))
	state_ids = cursor.fetchall()
	#print len(results)
	return state_ids


def get_normalized_states_from_state_ids(db_conn, state_ids, forces=False):
	cursor = db_conn.cursor()

	sequences = list()
	targets = list()

	force_data = list()

	means, variances = get_means_variances(db_conn)

	for id in state_ids:
		cursor.execute("SELECT * FROM STATES WHERE ID = {SID}".format(SID=id['STATE_ID']))
		state = cursor.fetchone()
		if state['GROUND_STIFFNESS'] == 0.0:
			continue

		feature, feet_forces = vectorize_state(state, means=means, variances=variances, include_forces=forces)

		sequences.append(feature)
		targets.append(
			(
				float(standardize_data(state['GROUND_SLOPE'], means['GROUND_SLOPE'], variances['GROUND_SLOPE'])),
				float(standardize_data(state['GROUND_STIFFNESS'], means['GROUND_STIFFNESS'], variances['GROUND_STIFFNESS']))
			)
		)
		if feet_forces[0]: # If not empty
			force_data.append(feet_forces)

	return sequences, targets, force_data


def get_differenced_states_from_state_ids(db_conn, state_ids, normalize=True, forces=False):

	cursor = db_conn.cursor()

	data = list()
	labels = list()
	force_data = list()

	means = None
	variances = None
	if normalize:
		means, variances = get_means_variances(db_conn)

	first_id = state_ids[0]
	cursor.execute("SELECT * FROM STATES WHERE ID = {SID}".format(SID=first_id['STATE_ID']))
	prev_state = cursor.fetchone()
	prev_vector, _ = vectorize_state(prev_state, means, variances)
	for id in state_ids[1:]:
		cursor.execute("SELECT * FROM STATES WHERE ID = {SID}".format(SID=id['STATE_ID']))
		curr_state = cursor.fetchone()
		if curr_state['GROUND_STIFFNESS'] == 0.0:
			continue
		curr_vector, forces = vectorize_state(curr_state, means, variances, include_forces=forces)
		data.append(np.subtract(curr_vector, prev_vector))

		#labels[0].append(float(standardize_data(curr_state['GROUND_SLOPE'], means['GROUND_SLOPE'], variances['GROUND_SLOPE'])))
		#labels[1].append(float(standardize_data(curr_state['GROUND_STIFFNESS'], means['GROUND_STIFFNESS'], variances['GROUND_STIFFNESS'])))

		labels.append(
			(
				float(standardize_data(curr_state['GROUND_SLOPE'], means['GROUND_SLOPE'], variances['GROUND_SLOPE'])),
			 	float(standardize_data(curr_state['GROUND_STIFFNESS'], means['GROUND_STIFFNESS'], variances['GROUND_STIFFNESS'])),
			 	#float(standardize_data(curr_state['GROUND_DAMPING'], means['GROUND_DAMPING'], variances['GROUND_DAMPING']))
			)
		)
		force_data.append(forces)
	return data, labels, force_data


def vectorize_state(state, means, variances, include_forces=False):
	vector = list()
	forces = (list(), list())
	if include_forces:
		# TODO
		#print 'Include forces'
		lf_forces = [float(f) for force in state['LF_FORCES'].split('|') for f in force.split(',')]
		rf_forces = [float(f) for force in state['RF_FORCES'].split('|') for f in force.split(',')]
		for i in range(0, len(lf_forces)):
			lf_force = standardize_data(lf_forces[i], means['LF_FORCES'][i], variances['LF_FORCES'][i])
			rf_force = standardize_data(rf_forces[i], means['RF_FORCES'][i], variances['RF_FORCES'][i])
			forces[0].append(lf_force)
			forces[1].append(rf_force)

	for k in state.keys():
		if k == 'LF_FORCES' \
			or k == 'RF_FORCES' \
			or k == 'ID' \
			or k == 'GROUND_DAMPING' \
			or k == 'GROUND_SLOPE' \
			or k == 'GROUND_STIFFNESS':
			continue
		feature = float(state[k])
		# print('k: ' + k)
		if means and variances:
			feature = standardize_data(feature, means[k], variances[k])
		vector.append(feature)
	return np.asarray(vector), forces


def prepare_data_for_sequences(data, labels, seq_length, full_sequence=True):
	sequences = list()
	targets = list()
	for i in range(0, len(data) - seq_length + 1):
		sequence = data[i: i + seq_length]
		if full_sequence:
			seq_labels = [[label] for label in labels[i:i+seq_length]]
			targets.append(seq_labels)
		else:
			targets.append([labels[seq_length - 1]])
		sequences.append(sequence)
	return sequences, targets


def prepare_forces_sequences(forces, seq_length):
	lf_force_sequence = list()
	rf_force_sequence = list()
	lf_forces, rf_forces = zip(*forces)
	for i in range (0, len(forces) - seq_length + 1):
		lf_force_sequence.append(lf_forces[i : i + seq_length])
		rf_force_sequence.append(rf_forces[i : i + seq_length])
	return lf_force_sequence, rf_force_sequence


def get_means_variances(db_conn):

	try:
		means = pickle.load(open('means.p', 'rb'))
		variances = pickle.load(open('variances.p', 'rb'))
	except Exception as e:
		print ('Means and variances do not exist')
		cursor = db_conn.cursor()
		means = {
			'TORSO_LV_X': 0.0,
			'TORSO_LV_Y': 0.0,
			'TORSO_D': 0.0,
			'TORSO_O': 0.0,
			'TORSO_AV': 0.0,
			'URL_D': 0.0,
			'URL_O': 0.0,
			'URL_AV': 0.0,
			'ULL_D': 0.0,
			'ULL_O': 0.0,
			'ULL_AV': 0.0,
			'LRL_D': 0.0,
			'LRL_O': 0.0,
			'LRL_AV': 0.0,
			'LLL_D': 0.0,
			'LLL_O': 0.0,
			'LLL_AV': 0.0,
			'RF_D': 0.0,
			'RF_O': 0.0,
			'RF_AV': 0.0,
			'LF_D': 0.0,
			'LF_O': 0.0,
			'LF_AV': 0.0,
			'LF_FORCES': [0.0] * 44,
			'RF_FORCES': [0.0] * 44,
			'GROUND_SLOPE': 0.0,
			'GROUND_STIFFNESS': 0.0,
			'GROUND_DAMPING': 0.0
		}
		variances = copy.deepcopy(means)

		# Iterate through entire table states and update the means
		cursor.execute('SELECT * FROM STATES')
		n = 1
		for row in cursor:
			lf_forces = [float(f) for force in row['LF_FORCES'].split('|') for f in force.split(',')]
			rf_forces = [float(f) for force in row['RF_FORCES'].split('|') for f in force.split(',')]

			for force in range(0, len(lf_forces)):

				LF_old_mean = means['LF_FORCES'][force]
				RF_old_mean = means['RF_FORCES'][force]
				means['LF_FORCES'][force] = update_mean(means['LF_FORCES'][force], lf_forces[force], n)
				means['RF_FORCES'][force] = update_mean(means['RF_FORCES'][force], rf_forces[force], n)

				variances['LF_FORCES'][force] = update_var(variances['LF_FORCES'][force], lf_forces[force], LF_old_mean, means['LF_FORCES'][force])
				variances['RF_FORCES'][force] = update_var(variances['RF_FORCES'][force], rf_forces[force], RF_old_mean, means['RF_FORCES'][force])

			del row['LF_FORCES']
			del row['RF_FORCES']

			# Update means for the rest of the fields
			for k in row:
				if k == 'ID':
					continue
				old_mean = means[k]
				means[k] = update_mean(means[k], row[k], n)
				variances[k] = update_var(variances[k], row[k], old_mean, means[k])
			n += 1

		for k in variances:
			if k == 'LF_FORCES' or k == 'RF_FORCES':
				variances[k][:] = [v/(n-1) for v in variances[k]]
			else:
				variances[k] /= (n-1)

		pickle.dump(means, open('means.p', 'wb'))
		pickle.dump(variances, open('variances.p', 'wb'))
	return means, variances


def update_mean(old_mean, val, n):
	return old_mean + (val - old_mean)/n


def update_var(old_var, val, old_mean, new_mean):
	return old_var + (val - old_mean) * (val - new_mean)


def build_feature_vec(state, db_conn, include_forces=False):

	feature_vec = list()
	label_vec = list()

	means, variances = get_means_variances(db_conn)

	if include_forces:
		lf_forces = [float(f) for force in state['LF_FORCES'].split('|') for f in force.split(',')]
		rf_forces = [float(f) for force in state['RF_FORCES'].split('|') for f in force.split(',')]

		std_lf_forces = list()
		std_rf_forces = list()

		for force in range (0, len(lf_forces)):
			std_lf_forces[force] = standardize_data(lf_forces[force], means['LF_FORCES'][force], variances['LF_FORCES'][force])
			std_rf_forces[force] = standardize_data(rf_forces[force], means['RF_FORCES'][force], variances['RF_FORCES'][force])

	for k in means.keys():
		if k == 'LF_FORCES' or k == 'RF_FORCES' or k == 'TORSO_LV_X' or k == 'TORSO_LV_Y':
			continue
		feature_vec.append(standardize_data(state[k], means[k], variances[k]))

	label_vec.append(standardize_data(state['GROUND_SLOPE'], means['GROUND_SLOPE'], variances['GROUND_SLOPE']))
	label_vec.append(standardize_data(state['GROUND_STIFFNESS'], means['GROUND_STIFFNESS'], variances['GROUND_STIFFNESS']))
	label_vec.append(standardize_data(state['GROUND_DAMPING'], means['GROUND_DAMPING'], variances['GROUND_DAMPING']))

	return feature_vec, label_vec


def standardize_data(dat, mean, variance):
	if variance == 0:
		return 0
	return (dat - mean) / math.sqrt(variance)


def dict_factory(cursor, row):
	d = {}
	for idx, col in enumerate(cursor.description):
		d[col[0]] = row[idx]
	return d


def prepare_data(db_str, num_seq=None, mode='difference', include_forces=False, dump=True):
	db_connection = initialize_db_connection(db_str)
	db_connection.row_factory = dict_factory
	num_sequences = get_number_of_sequences(db_connection)['MAX(SEQUENCE_ID)']
	xs = list()
	ys = list()
	foot_forces = (list(), list())
	if num_seq == None or num_seq >= num_sequences:
		num_seq = num_sequences
	for seq in range(1, num_seq):
		state_ids = get_sequence(db_connection, seq)
		forces = list()
		if mode=='difference':
			data, labels, forces = get_differenced_states_from_state_ids(db_connection, state_ids, forces=include_forces)
		elif mode=='normalize':
			data, labels, forces = get_normalized_states_from_state_ids(db_connection, state_ids, forces=include_forces)
		x, y = prepare_data_for_sequences(data, labels, seq_length=30, full_sequence=True)
		if forces:
			l_f, r_f = prepare_forces_sequences(forces, seq_length=30)
			foot_forces[0].extend(l_f)
			foot_forces[1].extend(r_f)
		xs.extend(x)
		ys.extend(y)
	if dump:
		if mode=='difference':
			pickle.dump(ys, open('SIMB_targets_diff.p', 'wb'))
			pickle.dump(xs, open('SIMB_sequences_diff.p', 'wb'))
		elif mode == 'normalize':
			pickle.dump(ys, open('SIMB_targets_normalized.p', 'wb'))
			pickle.dump(xs, open('SIMB_sequences_normalized.p', 'wb'))
		if include_forces:
			pickle.dump(foot_forces, open('SIMB_forces.p', 'wb'))
	db_connection.close()
	return xs, ys, foot_forces


def data_generator(seq_length,
				   db_connection,
				   max_seq_length = 30,
				   avg_window=4,
				   sample_size=32,
				   input_mode=0,
				   include_mode=0
				   ):
	num_sequences = get_number_of_sequences(db_connection)['MAX(SEQUENCE_ID)']
	# Get the size for each sequence?
	sizes = list()
	for sequence in range(1, num_sequences + 1):
		state_ids = get_sequence(db_connection, sequence)
		sizes.append(len(state_ids))
	ps = np.divide(sizes, float(np.sum(sizes)))
	choices = range(1, num_sequences + 1)
	while True:
		sample = list() # Fill this list with sample size
		slope_targets, ground_targets = list(), list()
		while len(sample) < sample_size:
			# Get random sequence based on size of sequence
			rand_seq = np.random.choice(
				choices,
				p=ps
			)
			state_ids = get_sequence(db_connection, rand_seq)

			if len(state_ids) > seq_length:
				# Start from random index in state_ids
				start_idx = np.random.randint(low=0, high=len(state_ids) - seq_length)
				state_ids = state_ids[start_idx:start_idx + seq_length]
				data, labels, forces = get_normalized_states_from_state_ids(
					db_connection,
					state_ids,
					forces=True
				)
				x, y = prepare_data_for_sequences(data, labels, seq_length=seq_length, full_sequence=True)
				l_f, r_f = prepare_forces_sequences(forces, seq_length=seq_length)
				l_f = avg_down_foot_forces(l_f, avg_window=avg_window)
				r_f = avg_down_foot_forces(r_f, avg_window=avg_window)
				forces = np.concatenate((l_f, r_f), axis=2)  # Concatenate the forces
				if input_mode == 0:
					inputs = np.concatenate((x, forces), axis=2)
				elif input_mode == 1: # No Forces
					inputs = np.asarray(x)
				elif input_mode == 2: # No State
					inputs = np.asarray(forces)
				slope_target, ground_target = split_targets(y[0])
				slope_target = np.array(slope_target)
				ground_target = np.array(ground_target)
				datum = np.zeros(shape=(max_seq_length, inputs.shape[2]))
				datum[:seq_length] = inputs[:]
				sample.append(datum)
				slope_y = np.zeros(shape=(max_seq_length, slope_target.shape[1]))
				slope_y[:seq_length] = slope_target[:]
				slope_targets.append(slope_y)
				ground_y = np.zeros(shape=(max_seq_length, 1))
				ground_y[:seq_length] = ground_target[:]
				ground_targets.append(ground_y)
		if include_mode == 0:
			yield (
				{
					'input_1': np.asarray(sample)
				},
				{
					'slope_output': np.asarray(slope_targets),
					'compliance_output': np.asarray(ground_targets)
				}
			)
		elif include_mode == 1:
			yield (
				{
					'input_1': np.asarray(sample)
				},
				{
					'slope_output': np.asarray(slope_targets)
				}
			)
		elif include_mode == 2:
			yield (
				{
					'input_1': np.asarray(sample)
				},
				{
					'compliance_output': np.asarray(ground_targets)
				}
			)


def avg_down_foot_forces(forces, avg_window=4):
	avg_force_data = list()
	for force_sequence in forces:
		avg_force_sequence = list()
		for forces in force_sequence:
			avg_forces = list()
			for i in range(0, int(math.ceil(len(forces)/avg_window))):
				end_idx = i * avg_window + avg_window
				if end_idx > len(forces):
					end_idx = len(forces)
				avg_force = np.mean(forces[i*avg_window : end_idx])
				avg_forces.append(avg_force)
			avg_force_sequence.append(avg_forces)

		avg_force_data.append(avg_force_sequence)
	return avg_force_data


def split_targets(targets):
	slope_targets = list()
	ground_targets = list()
	for target in targets:
		labels = target[0]
		slope_targets.append([labels[0]])
		ground_targets.append([labels[1]])
	return slope_targets, ground_targets


def split_labels(labels):
	targets = [list(), list()]
	for label in labels:
		slopes, grounds = split_targets(label)
		targets[0].append(slopes)
		targets[1].append(grounds)
	return targets


def unstandardize_labels(slopes, stiffnesses, dampings):

	means = pickle.load(open('means.p', 'rb'))
	variances = pickle.load(open('variances.p', 'rb'))

	if slopes:
		slopes = map(lambda p: unstandardize_data(p, means['GROUND_SLOPE'], variances['GROUND_SLOPE']), slopes)
	if stiffnesses:
		stiffnesses = map(lambda p: unstandardize_data(p, means['GROUND_STIFFNESS'], variances['GROUND_STIFFNESS']), stiffnesses)
	if dampings:
		dampings = map(lambda p: unstandardize_data(p, means['GROUND_DAMPING'], variances['GROUND_DAMPING']), dampings)

	return slopes, stiffnesses, dampings


def unstandardize_data(data, mean, variance):
	return data * math.sqrt(variance) + mean


if __name__ == "__main__":

	prepare_data('samples_33.db', num_seq=30, mode='normalize', include_forces=True, dump=False)

	# db_connection = initialize_db_connection("samples_w_compliance.db")
	# db_connection.row_factory = dict_factory
	# generator = data_generator(
	# 	seq_length=30,
	# 	db_connection=db_connection,
	# 	input_mode=2
	# )
	#
	# for _ in range(0, 3):
	# 	next(generator)

	# v_db_connection = initialize_db_connection('samples_w_compliance_validation.db')
	# v_db_connection.row_factory = dict_factory
	# valid_gen = data_generator(
	# 	seq_length=30,
	# 	db_connection=v_db_connection,
	# 	avg_window=4,
	# 	sample_size=5
	# )
	#
	# for i in range(0, 4):
	# 	next(generator)
	# 	next(valid_gen)