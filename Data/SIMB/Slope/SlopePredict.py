import time
from numpy import newaxis
import numpy as np
from keras.models import Model
from keras.layers import Input
from keras.layers.core import Dense, Activation, Dropout, Masking, Reshape, TimeDistributedDense, Merge
from keras.layers.recurrent import LSTM
from keras.layers.wrappers import TimeDistributed
from keras.layers.pooling import  AveragePooling1D
from keras.models import load_model
import matplotlib.pyplot as plt
import matplotlib.colors as colors
from keras.utils.np_utils import to_categorical
import os
import math

import DatabaseInterface as DBI

import matplotlib.lines as lines
import cPickle as pickle

from random import shuffle
import imp

cwd = os.getcwd()

while True:
	if cwd.split('/')[-1] == 'SIMB':
		break
	else:
		l = cwd.split('/')[:-1]
		s = '/'
		cwd = s.join(l)

data_path = cwd

BaseClass = imp.load_source('BaseClass', data_path + '/BaseClass.py')
Base_Model = BaseClass.Base_Model

class LSTM_Predictor(Base_Model):

	def __init__(self, lstm_layers, num_outputs, num_features, max_seq_length, output_name, dir='.', save_substr='K_SIMB'):

		Base_Model.__init__(
			self,
			num_features=num_features,
			max_seq_length=max_seq_length,
			save_substr=save_substr,
			dir=dir
		)

		if not self.loaded:

			in_layer = Input(
				shape=(max_seq_length, num_features),
				name='input_1'
			)

			common_layer = Masking(mask_value=0., input_shape=(max_seq_length, num_features))(in_layer)

			# LSTM Layers
			for layer in lstm_layers:
				common_layer = LSTM(
					output_dim=layer,
					return_sequences=True
				)(common_layer)
				common_layer = Dropout(
					p=0.2
				)(common_layer)

			# Output layers
			slope_output = TimeDistributed(
				Dense(
					output_dim=num_outputs,
					activation='linear'
				),
				name=output_name
			)(common_layer)

			self.model = Model(
				input=[in_layer],
				output=[slope_output]
			)

			start = time.time()

			self.model.compile(
				optimizer='rmsprop',
				loss='mse'
			)

			print "Compilation Time : ", time.time() - start

			print 'model layers: '
			print self.model.summary()

			print 'model.inputs: '
			print self.model.input_shape

			print 'model.outputs: '
			print self.model.output_shape

	def train_on_generator(self, data_generator, valid_generator, samples_per_epoch=1500, nb_epoch=50, continue_training=True):
		Base_Model.train_on_generator(self, data_generator, valid_generator, samples_per_epoch, nb_epoch, continue_training)

	def predict_on_batch(self, gen, num_samples, title, output_name):
		plt.ion()
		plt.figure(1)
		plt.subplot(111)
		plt.title(title)
		plt.tight_layout()
		errors = list()
		for sample in range(0, num_samples):
			batch, labels = next(gen)
			preds = self.model.predict_on_batch(
				x=batch
			)
			actual = labels[output_name]
			for pred in range(0, len(preds)):
				slope_preds = preds[pred]
				slope_preds = [item for sublist in slope_preds for item in sublist]  # Flatten
				slope_actual = actual[pred]
				slope_actual = [item for sublist in slope_actual for item in sublist]  # Flatten

				slopes_p, stiffnesses_p, dampings_p = DBI.unstandardize_labels(slope_preds, None, None)
				slopes_a, stiffnesses_a, dampings_a = DBI.unstandardize_labels(slope_actual, None, None)

				idx = sample * len(preds) + pred

				plt.subplot(111)
				plt.scatter(x=idx, y=slopes_p[-1], c='b', alpha=0.65)
				plt.scatter(x=idx, y=slopes_a[-1], c='r')
				# Draw a line.
				plt.plot([idx, idx], [slopes_p[-1], slopes_a[-1]], color='k')
				plt.pause(0.01)
				errors.append(slopes_a[-1] - slopes_p[-1])
		pickle.dump(errors, open('Errors.p', 'wb'))
		plt.waitforbuttonpress()
		plt.hist(x=errors, bins=15)
		plt.waitforbuttonpress()

	def train_on_data(self, data, labels, batch_size, epochs, continue_training=True):
		if continue_training:
			Base_Model.train_on_data(self, data, labels, batch_size, epochs)

	def predict_on_data(self, data, labels):

		predictions = self.model.predict_on_batch(
			x=data
		)

		plt.figure(1)
		plt.subplot(111)
		plt.tight_layout()
		plt.title('Slope predictions')
		plt.ion()
		for i, (ps, labels) in enumerate(zip(predictions, labels)):

			p = [item for sublist in ps for item in sublist]
			a = [item for sublist in labels for item in sublist]

			p, _, _ = DBI.unstandardize_labels(p, None, None)
			a, _, _ = DBI.unstandardize_labels(a, None, None)

			yhat = p[-1]
			ytrue = a[-1]
			plt.subplot(111)
			plt.tight_layout()
			plt.scatter(i, ytrue, c='r')
			plt.scatter(i, yhat, c='b', alpha=0.6)
			# Draw a line
			plt.plot([i, i], [yhat, ytrue], color='k')
			plt.pause(0.01)
		plt.waitforbuttonpress()

if __name__ == '__main__':
	# Testing
	LSTM_Predictor(
		lstm_layers=[128, 128],
		num_outputs=1,
		num_features=23,
		max_seq_length=30,
		output_name='slope_output',
		save_substr='YOOOOUUU'
	)