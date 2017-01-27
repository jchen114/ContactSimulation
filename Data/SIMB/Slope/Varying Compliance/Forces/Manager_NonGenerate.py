import imp
import cPickle as pickle
import numpy as np
# Inside SIMB/Slope/Varying Compliance/Forces

if __name__ == '__main__':
	module = imp.load_source('SlopePredict', '../../SlopePredict.py')

	DBI = imp.load_source('DatabaseInterface', '../../../../DatabaseInterface.py')

	try:
		data = pickle.load(open('SIMB_sequences_normalized.p', 'rb'))
		labels = pickle.load(open('SIMB_targets_normalized.p', 'rb'))
		forces = pickle.load(open('SIMB_forces.p', 'rb'))
	except Exception:
		data, labels, forces = DBI.prepare_data(
			db_str='../../../../samples_w_compliance.db',
			num_seq=None,
			mode='normalize',
			include_forces=True,
			dump=True
		)

	lf_forces = forces[0]
	rf_forces = forces[1]

	rf_forces = DBI.avg_down_foot_forces(lf_forces)
	lf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	# Combine Data with Foot Forces
	data = np.concatenate((data, forces), 2)

	targets = DBI.split_labels(labels)

	slopes = targets[0]

	predictor = module.LSTM_Predictor(
		lstm_layers=[128, 128],
		num_outputs=1,
		num_features=np.shape(data)[-1],
		max_seq_length=30,
		output_name='slope_output',
		save_substr='model-normalized'
	)

	#train_data = data[:int(0.8*len(data))]
	#train_labels = slopes[:int(0.8*len(slopes))]

	predictor.train_on_data(
		data=data,
		labels=slopes,
		batch_size=32,
		epochs=50,
		continue_training=True
	)

	#test_data = data[int(0.8*len(data)) + 1 :]
	#test_labels = slopes[int(0.8*len(slopes)) + 1 :]

	test_data, test_labels, forces = DBI.prepare_data(
		db_str='../../../../samples_w_compliance_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=False,
		dump=False
	)

	#test_data = test_data[int(0.8*len(test_data)) + 1:]
	#test_labels = test_labels[int(0.8*len(test_labels)) + 1:]

	targets = DBI.split_labels(test_labels)

	test_labels = targets[0]

	predictor.predict_on_data(test_data[:800], test_labels[:800])
