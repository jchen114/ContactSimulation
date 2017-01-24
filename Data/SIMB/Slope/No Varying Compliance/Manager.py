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
		save_substr='model'
	)

	DBI = imp.load_source('DatabaseInterface', '../../../DatabaseInterface.py')

	try:
		data = pickle.load(open('SIMB_sequences_diff.p', 'rb'))
		labels = pickle.load(open('SIMB_targets_diff.p', 'rb'))
	except Exception:
		data, labels = DBI.prepare_data(
			db_str='../../../samples.db',
			num_seq=None,
			difference=True,
			include_forces=False,
			dump=True
		)

	targets = DBI.split_labels(labels)

	slopes = targets[0]

	predictor.train_on_data(
		data=data,
		labels=slopes,
		batch_size=32,
		epochs=50,
		continue_training=False
	)

	test_data, test_labels = DBI.prepare_data(
		db_str='../../../samples_test.db',
		num_seq=None,
		difference=True,
		include_forces=False,
		dump=False
	)

	targets = DBI.split_labels(test_labels)

	slopes = targets[0]

	predictor.predict_on_data(test_data[:300], slopes[:300])
