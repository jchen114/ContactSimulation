import time
from numpy import newaxis
import numpy as np
from keras.models import Model
from keras.layers import Input
from keras.layers.core import Dense, Activation, Dropout, Masking, Reshape
from keras.layers.recurrent import LSTM
from keras.layers.wrappers import TimeDistributed
from keras.layers.pooling import  AveragePooling1D
from keras.models import load_model
from keras.callbacks import ModelCheckpoint, EarlyStopping
from keras.models import Sequential
import matplotlib.pyplot as plt
import matplotlib.colors as colors
from keras.utils.np_utils import to_categorical
import os
import math
import re

import DatabaseInterface as DBI

import matplotlib.lines as lines
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

from random import shuffle
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

	def __init__(self, lstm_layers, slope_dense_layers, ground_dense_layers, num_features, max_seq_length, save_substr, dir='.'):

		BaseClass.Base_Model.__init__(self, num_features, max_seq_length, save_substr, dir)

		if not self.loaded:
			in_layer = Input(
				shape=(max_seq_length, num_features),
				name='input_1'
			)

			common_layer = Masking(mask_value=0., input_shape=(max_seq_length, num_features))(in_layer)

			# LSTM Layers Common layers
			for layer in lstm_layers:

				common_layer = LSTM(
					output_dim=layer,
					return_sequences=True
				)(common_layer)
				common_layer = Dropout(
					p=0.2
				)(common_layer)

			slope_layer = common_layer
			ground_layer = common_layer

			#======= SPLIT ======#

			# Slopes
			for layer in slope_dense_layers:
				slope_layer = TimeDistributed (
					Dense (
						output_dim=layer,
						activation='relu'
					)
				)(slope_layer)
				slope_layer = Dropout(
					p=0.2
				)(slope_layer)

			# Ground Properties
			for layer in ground_dense_layers:
				ground_layer = TimeDistributed (
					Dense (
						output_dim=layer,
						activation='relu'
					)
				)(ground_layer)
				ground_layer = Dropout (
					p=0.2
				)(ground_layer)

			# Output layers
			slope_output = TimeDistributed(
				Dense (
					output_dim=1,
					activation='linear'
				),
				name='slope_output'
			)(slope_layer)

			ground_output = TimeDistributed(
				Dense (
					output_dim=1,
					activation='linear'
				),
				name='compliance_output'
			)(ground_layer)

			self.model = Model(
				input=[in_layer],
				output=[slope_output, ground_output]
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

	def predict_on_batch(self, generator, num_samples):
		plt.ion()
		plt.figure(1)
		plt.subplot(311)
		plt.title('Slope')
		plt.subplot(312)
		plt.title('Stiffness')
		plt.subplot(313)
		plt.title('Damping')
		for sample in range(0, num_samples):
			batch, labels = next(generator)
			preds = self.model.predict_on_batch(
				x=batch
			)
			slope_preds = preds[0]
			ground_preds = preds[1]
			slope_actual = labels['slope_output']
			ground_actual = labels['ground_output']
			for pred in range(0, len(slope_preds)):
				slopes_p = slope_preds[pred]
				slopes_p = [item for sublist in slopes_p for item in sublist] # Flatten
				slopes_a = slope_actual[pred]
				slopes_a = [item for sublist in slopes_a for item in sublist] # Flatten

				grounds_p = ground_preds[pred]
				grounds_a = ground_actual[pred]

				grounds_p = [(stiffness, damping)  for [stiffness, damping] in grounds_p]
				grounds_a = [(stiffness, damping) for [stiffness, damping] in grounds_a]
				stiffnesses_p, dampings_p = zip(*grounds_p)
				stiffnesses_a, dampings_a = zip(*grounds_a)

				slopes_p, stiffnesses_p, dampings_p = DBI.unstandardize_labels(slopes_p, stiffnesses_p, dampings_p)
				slopes_a, stiffnesses_a, dampings_a = DBI.unstandardize_labels(slopes_a, stiffnesses_a, dampings_a)

				idx = sample * len(slope_preds) + pred

				plt.subplot(311)
				plt.scatter(x=idx, y=slopes_p[-1], c='b', alpha=0.65)
				plt.scatter(x=idx , y=slopes_a[-1], c='r')
				plt.subplot(312)
				plt.scatter(x=idx, y=stiffnesses_p[-1], c='b', alpha=0.65)
				plt.scatter(x=idx, y=stiffnesses_a[-1], c='r')
				plt.subplot(313)
				plt.scatter(x=idx, y=dampings_p[-1], c='b', alpha=0.65)
				plt.scatter(x=idx, y=dampings_a[-1], c='r')
				plt.pause(0.01)
		plt.waitforbuttonpress()

	def predict_on_data(self, data, slope_labels, compliance_labels):


		predictions = self.model.predict_on_batch(
			x=data
		)

		slope_preds = predictions[0]
		ground_preds = predictions[1]

		slope_errors = []
		compliance_errors = []

		# plt.ion()
		# plt.figure(1)
		# plt.subplot(211)
		# plt.title('Slope')
		# plt.subplot(212)
		# plt.title('Compliance')

		for prediction in range(0, len(slope_preds)):

			slopes = slope_preds[prediction]
			slopes_actual = slope_labels[prediction]
			#
			compliances = ground_preds[prediction]
			compliances_actual = compliance_labels[prediction]
			#
			# plt.subplot(211)
			# plt.scatter(prediction, slopes[-1], c='b', alpha=0.6)
			# plt.scatter(prediction, slopes_actual[-1], c='r')
			#
			# plt.plot([prediction, prediction], [slopes[-1], slopes_actual[-1]], color='k')
			#
			# plt.subplot(212)
			# plt.scatter(prediction, compliances[-1], c='b', alpha=0.6)
			# plt.scatter(prediction, compliances_actual[-1], c='r')
			#
			# plt.plot([prediction, prediction], [compliances[-1], compliances_actual[-1]], color='k')
			slope_errors.append(abs(slopes_actual[-1] - slopes[-1]))
			compliance_errors.append(abs(compliances_actual[-1] - compliances[-1]))

		#plt.waitforbuttonpress()
		#plt.show()

		plt.figure(2)
		plt.subplot(211)
		plt.title('Slope Error Histogram')
		plt.hist(
			x=slope_errors,
			bins=20,
			normed=True
		)
		plt.xlabel('Errors')
		plt.ylabel('Frequency')
		plt.show()

		plt.subplot(212)
		plt.title('Compliance Error Histogram')
		plt.hist(
			x=compliance_errors,
			bins=20,
			normed=True
		)
		plt.xlabel('Errors')
		plt.ylabel('Frequency')
		plt.show()



#if __name__ == '__main__':

	# data = pickle.load(open('SIMB_sequences_diff_train.p', 'rb'))
	# labels = pickle.load(open('SIMB_targets_diff_traind.p', 'rb'))

	#labels = DBI.split_labels(labels)
	#pickle.dump(labels, open('SIMB_targets_diff.p', 'wb'))

	# Slope_Predictor = LSTM_Predictor(
	# 	lstm_layers=[128, 128],
	# 	num_outputs=1,
	# 	num_features=23,
	# 	max_seq_length=30,
	# 	output_name='slope_output',
	# 	save_substr='Keras-SIMB_Slope_no_forces_no_generator',
	# )

	# Predict on slope only.
	# slopes = labels[0]
	# Slope_Predictor.train_on_data(
	# 	data=data,
	# 	labels=slopes,
	# 	batch_size=32,
	# 	epochs=50,
	# 	continue_training=False
	# )

	# test_data = pickle.load(open('SIMB_sequences_diff_test.p', 'rb'))
	# test_labels = pickle.load(open('SIMB_targets_diff_test.p', 'rb'))
	#
	# Slope_Predictor.predict_on_data(test_data, test_labels)

	# errors = pickle.load(open('Errors.p', 'rb'))
	# plt.hist(errors)
	# plt.show()
	#
	# forces_dim = 22
	# state_dim = 23
	#
	# db_connection = DBI.initialize_db_connection("samples_w_compliance.db")
	# db_connection.row_factory = DBI.dict_factory
	# data_gen = DBI.data_generator(
	# 	seq_length=30,
	# 	db_connection=db_connection,
	# 	avg_window=4,
	# 	sample_size=32,
	# 	include_compliance=False,
	# 	include_compliance_targets=False
	# )
	#
	# v_db_connection = DBI.initialize_db_connection('samples_w_compliance_validation.db')
	# v_db_connection.row_factory = DBI.dict_factory
	#
	# valid_gen = DBI.data_generator(
	# 	seq_length=30,
	# 	db_connection=v_db_connection,
	# 	avg_window=4,
	# 	sample_size=20,
	# 	include_compliance=False,
	# 	include_compliance_targets=False
	# )
	#
	# t_db_connection = DBI.initialize_db_connection('samples_w_compliance_test.db')
	# t_db_connection.row_factory = DBI.dict_factory
	#
	# test_gen = DBI.data_generator(
	# 	seq_length=31,
	# 	db_connection=t_db_connection,
	# 	avg_window=4,
	# 	sample_size=15,
	# 	include_compliance=False,
	# 	include_compliance_targets=False
	# )
	#
	#
	# Slope_Predictor.train_on_generator(
	# 	data_generator=data_gen,
	# 	valid_generator=valid_gen,
	# 	samples_per_epoch=3000,
	# 	nb_epoch=50,
	# 	continue_training=True
	# )
	#
	# Slope_Predictor.predict_on_batch(
	# 	gen=test_gen,
	# 	num_samples=33,
	# 	title='Slope',
	# 	output_name='slope_output'
	# )

	# S_GC_predictor = Slope_Ground_Predictor(
	# 	lstm_layers=[128, 128],
	# 	slope_dense_layers=[32],
	# 	ground_dense_layers=[32, 32],
	# 	num_features=45,
	# 	max_seq_length=29,
	# 	save_substr='Keras-Slope+Ground_Compliance'
	# )
	#
	# S_GC_predictor.train_on_generator(
	# 	data_generator=data_generator,
	# 	valid_generator=valid_gen,
	# 	samples_per_epoch=1500,
	# 	nb_epoch=50,
	# 	continue_training=False
	# )
	#
	# S_GC_predictor.predict_on_batch(
	# 	generator=test_gen,
	# 	num_samples=33
	# )


