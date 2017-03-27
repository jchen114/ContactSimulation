import sys
maj = sys.version_info

import numpy as np

version = 2

if maj[0] >= 3:
	import _pickle as pickle
	import importlib.machinery
	import types
	version = 3
else:
	import cPickle as pickle
	import imp

# Inside SIMB/Slope + Compliance/Separate Network

import LSTM_Model

if __name__ == '__main__':

	if version == 3:
		loader = importlib.machinery.SourceFileLoader('DatabaseInterface', '../../../DatabaseInterface.py')
		DBI = types.ModuleType(loader.name)
		loader.exec_module(DBI)
	else:
		DBI = imp.load_source('DatabaseInterface', '../../../DatabaseInterface.py')

	# ========================= DATABASE SETUP ========================= #

	data_connection = DBI.initialize_db_connection('../../../samples_33.db')
	data_connection.row_factory = DBI.dict_factory
	slope_data_gen = DBI.data_generator(
		seq_length=30,
		db_connection=data_connection,
		mode='normalize',
		include_mode=1
	)

	# Validation
	try:
		valid_data = pickle.load(open('SIMB_sequences_normalized_val.p', 'rb'))
		valid_labels = pickle.load(open('SIMB_targets_normalized_val.p', 'rb'))
		foot_forces = pickle.load(open('SIMB_forces_val.p', 'rb'))
	except:
		valid_data, valid_labels, foot_forces = DBI.prepare_data(
			db_str='../../../samples_33_val.db',
			num_seq=None,
			mode='normalize',
			include_forces=True,
			dump=True
		)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	valid_data = np.concatenate((valid_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(valid_labels)

	# # ========================== BUILD NETWORKS ========================= #
	#
	slope_network = LSTM_Model.Network(
		lstm_layers=[128,128],
		dense_layers=[16],
		num_features=45,
		num_outputs=1,
		max_seq_length=30,
		output_name='slope_output',
		save_substr='model',
		dir='slope'
	)
	#
	# ================== TRAINING =================== #

	slope_network.train_on_generator_validation_set(
		continue_training=True,
		data_gen=slope_data_gen,
		samples_per_epoch=18100,
		nb_epoch=40,
		valid_data=(
			{
				'input_1': np.asarray(valid_data)
			},
			{
				'slope_output': np.asarray(slope_labels)
			}
		),
	)

	#
	# # ==================== TESTING =================== #
	#
	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_val.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	slope_network.predict_on_data(
		data=test_data[3000:5000],
		labels = slope_labels[3000:5000],
		title='Slopes'
	)

