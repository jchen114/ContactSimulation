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

	network = SGC.Slope_Ground_Predictor(
		lstm_layers=[128,128],
		slope_dense_layers=[32],
		ground_dense_layers=[32],
		num_features=45,
		max_seq_length=30,
		save_substr='model'
	)

