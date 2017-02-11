import sys
from keras.layers import Input, Masking, Dense, TimeDistributed, Dropout, LSTM
from keras.models import Model
import os
import re
import time
import matplotlib.pyplot as plt

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


cwd = os.getcwd()
split_str = '\\\\+|\/'
while True:
	tokens = re.split(split_str , cwd)
	if tokens[-1] == 'SIMB':
		break
	else:
		l = tokens[:-1]
		s = '/'
		cwd = s.join(l)

data_path = cwd

if version == 3:
	loader = importlib.machinery.SourceFileLoader('BaseClass', data_path + '/BaseClass.py')
	BaseClass = types.ModuleType(loader.name)
	loader.exec_module(BaseClass)
else:
	BaseClass = imp.load_source('BaseClass', data_path + '/BaseClass.py')


class Network(BaseClass.Base_Model):

	def __init__(self, lstm_layers, dense_layers, output_name, num_features, num_outputs, max_seq_length, save_substr, dir='.'):

		BaseClass.Base_Model.__init__(self, num_features, max_seq_length, save_substr, dir)

		if not self.loaded:

			in_layer = Input(
				shape=(max_seq_length, num_features),
				name='input_1'
			)

			net_layer = Masking(mask_value=0., input_shape=(max_seq_length, num_features))(in_layer)

			# LSTM Layers
			for layer in lstm_layers:
				net_layer = LSTM(
					output_dim=layer,
					return_sequences=True
				)(net_layer)
				net_layer = Dropout(
					p=0.2
				)(net_layer)

			# Dense Layers
			for layer in dense_layers:
				net_layer = TimeDistributed (
					Dense(
						output_dim=layer
					)
				)(net_layer)
				net_layer = Dropout (
					p=0.2
				)(net_layer)


			# Output layers
			output = TimeDistributed(
				Dense(
					output_dim=num_outputs,
					activation='linear'
				),
				name=output_name
			)(net_layer)

			self.model = Model(
				input=[in_layer],
				output=[output]
			)

			start = time.time()

			self.model.compile(
				optimizer='rmsprop',
				loss='mse'
			)

			print("Compilation Time : ", time.time() - start)

			print('model layers: ')
			print(self.model.summary())

			print('model.inputs: ')
			print(self.model.input_shape)

			print('model.outputs: ')
			print(self.model.output_shape)

	def predict_on_data(self, data, labels, title):
		plt.ion()
		plt.figure(1)
		plt.subplot(111)
		plt.title(title)

		predictions = self.model.predict_on_batch(
			x=data
		)

		for prediction in range(0, len(predictions)):
			p_vec = predictions[prediction]
			a_vec = labels[prediction]

			plt.subplot(111)
			plt.scatter(prediction, p_vec[-1], c='b', alpha=0.6)
			plt.scatter(prediction, a_vec[-1], c='r')

			plt.plot([prediction, prediction], [p_vec[-1], a_vec[-1]], color='k')
			plt.pause(0.01)
		plt.waitforbuttonpress()

	def _history(self, history):
		pickle.dump(history, open('history.p', 'wb'))