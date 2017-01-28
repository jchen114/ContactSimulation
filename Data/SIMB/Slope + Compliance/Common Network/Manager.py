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

import Slope_Compliance as SGC

# Inside SIMB/Slope + Compliance/Common Network

if __name__ == '__main__':

	if version == 3:
		loader = importlib.machinery.SourceFileLoader('DatabaseInterface', '../../../DatabaseInterface.py')
		DBI = types.ModuleType(loader.name)
		loader.exec_module(DBI)
	else:
		DBI = imp.load_source('DatabaseInterface', '../../../DatabaseInterface.py')

	data_connection = DBI.initialize_db_connection('../../../samples_w_compliance.db')
	data_connection.row_factory = DBI.dict_factory
	data_gen = DBI.data_generator(
		seq_length=30,
		db_connection=data_connection,
		mode='normalize',
		include_compliance_targets=True
	)

	valid_connection = DBI.initialize_db_connection('../../../samples_w_compliance_validation.db')
	valid_connection.row_factory = DBI.dict_factory
	valid_gen = DBI.data_generator(
		seq_length=30,
		db_connection=valid_connection,
		mode='normalize',
		include_compliance_targets=True
	)


	network = SGC.Slope_Ground_Predictor(
		lstm_layers=[128,128],
		slope_dense_layers=[32],
		ground_dense_layers=[32],
		num_features=45,
		max_seq_length=30,
		save_substr='model'
	)

	network.train_on_generator(
		data_generator=data_gen,
		valid_generator=valid_gen,
		samples_per_epoch=1500,
		nb_epoch=50,
		continue_training=True
	)