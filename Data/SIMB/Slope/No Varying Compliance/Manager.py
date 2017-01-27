import imp
import cPickle as pickle
# Inside SIMB/Slope/No Varying Compliance

if __name__ == '__main__':

	module = imp.load_source('SlopePredict', '../SlopePredict.py')

	predictor = module.LSTM_Predictor(
		lstm_layers=[128, 128],
		num_outputs=1,
		num_features=23,
		max_seq_length=30,
		output_name='slope_output',
		save_substr='model_normalize'
	)

	DBI = imp.load_source('DatabaseInterface', '../../../DatabaseInterface.py')

	try:
		data = pickle.load(open('SIMB_sequences_normalized.p', 'rb'))
		labels = pickle.load(open('SIMB_targets_normalized.p', 'rb'))
	except Exception:
		data, labels = DBI.prepare_data(
			db_str='../../../samples.db',
			num_seq=None,
			mode='normalize',
			include_forces=False,
			dump=True
		)

	targets = DBI.split_labels(labels)

	labels = targets[0]

	train_data = data[:int(0.9 * len(data))]
	train_labels = labels[:int(0.9*len(labels))]

	predictor.train_on_data(
		data=train_data,
		labels=train_labels,
		batch_size=32,
		epochs=50,
		continue_training=False
	)

	test_data, test_labels = DBI.prepare_data(
		db_str='../../../samples_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=False,
		dump=False
	)

	targets = DBI.split_labels(test_labels)

	test_labels = targets[0]

	#test_data = data[int(0.9 * len(train_data)) + 1:]
	#test_labels = labels[int(0.9 * len(train_data)) + 1 :]

	predictor.predict_on_data(test_data[:800], test_labels[:800])
