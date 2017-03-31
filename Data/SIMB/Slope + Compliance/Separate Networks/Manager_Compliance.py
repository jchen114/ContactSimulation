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

	compliance_data_gen = DBI.data_generator(
		seq_length=30,
		db_connection=data_connection,
		mode='normalize',
		include_mode=2
	)

	# Validation
	valid_connection = DBI.initialize_db_connection('../../../samples_w_compliance_validation.db')
	valid_connection.row_factory = DBI.dict_factory

	compliance_valid_gen = DBI.data_generator(
		seq_length=30,
		db_connection=valid_connection,
		mode='normalize',
		include_mode=2
	)

	# ========================== BUILD NETWORKS ========================= #

	compliance_network = LSTM_Model.Network(
		lstm_layers=[128, 128],
		dense_layers=[128],
		num_features=45,
		num_outputs=1,
		max_seq_length=30,
		output_name='compliance_output',
		save_substr='compliance_model'
	)

	# ================== TRAINING =================== #

	compliance_network.train_on_generator(
		data_generator=compliance_data_gen,
		valid_generator=compliance_valid_gen,
		samples_per_epoch=2000,
		nb_epoch=50,
		continue_training=False
	)

	# ==================== TESTING =================== #

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_w_compliance_test.db',
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
		data=test_data[:800],
		labels = slope_labels[:800],
		title='Slopes'
	)

	compliance_network.predict_on_data(
		data=test_data[:800],
		labels=compliance_labels[:800],
		title='Compliance'
	)
