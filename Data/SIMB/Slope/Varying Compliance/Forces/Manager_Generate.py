import imp
import cPickle as pickle
import numpy as np

# Inside SIMB/Slope/Varying Compliance/Forces

if __name__ == '__main__':

	# ========================== TRAINING ============================= #

	module = imp.load_source('SlopePredict', '../../SlopePredict.py')
	DBI = imp.load_source('DatabaseInterface', '../../../../DatabaseInterface.py')

	data_connection = DBI.initialize_db_connection('../../../../samples_w_compliance.db')
	data_connection.row_factory = DBI.dict_factory
	data_gen = DBI.data_generator(
		seq_length=30,
		db_connection=data_connection,
		mode='normalize',
		include_compliance_targets=False
	)

	valid_connection = DBI.initialize_db_connection('../../../../samples_w_compliance_validation.db')
	valid_connection.row_factory = DBI.dict_factory
	valid_gen = DBI.data_generator(
		seq_length=30,
		db_connection=valid_connection,
		mode='normalize',
		include_compliance_targets=False
	)

	predictor = module.LSTM_Predictor(
		lstm_layers=[128, 128],
		num_outputs=1,
		num_features=45,
		max_seq_length=30,
		output_name='slope_output',
		save_substr='model-normalized'
	)

	predictor.train_on_generator(
		data_generator=data_gen,
		valid_generator=valid_gen,
		samples_per_epoch=1500,
		nb_epoch=50,
		continue_training=False
	)

	# ==================== TESTING =================== #
	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../../samples_w_compliance_test.db',
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

	test_labels, _ = DBI.split_labels(test_labels)

	predictor.predict_on_data(
		data=test_data[:800],
		labels=test_labels[:800]
	)
