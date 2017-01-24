import time
from numpy import newaxis
from keras.layers.core import Dense, Activation, Dropout, Masking, Reshape, TimeDistributedDense
from keras.layers.recurrent import LSTM
from keras.layers.wrappers import TimeDistributed
from keras.models import load_model
from keras.callbacks import ModelCheckpoint, EarlyStopping
from keras.models import Sequential
import matplotlib.pyplot as plt
import matplotlib.colors as colors
from keras.utils.np_utils import to_categorical
import os

import DatabaseInterface as DI

import math
import numpy as np
from random import shuffle
import cPickle as pickle


class Keras_Model():

	def __init__(self, num_features, save_substr):
		self.num_features = num_features
		self.loaded = False
		self.return_sequences = False
		self.substr = save_substr
		files = [f for f in os.listdir('.') if os.path.isfile(f)]
		for f in files:
			if f.startswith(save_substr):
				print f
				self.model = load_model(f)
				self.loaded = True
				break

	def train_with_data(self, data, labels, batch_size, epochs):
		if not self.loaded:

			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=60
			)
			callbacks = [checkpoint, earlyStopping]
			data = np.asarray(data)
			labels = np.asarray(labels)

			self.model.fit(
				x=data,
				y=labels,
				batch_size=batch_size,
				nb_epoch=epochs,
				validation_split=0.05,
				callbacks=callbacks
			)

	def predict_on_data(self, data, labels, plot=True):
		bottom_errors = list()
		middle_errors = list()
		top_errors = list()
		plt.ion()
		for i, (sequence, label) in enumerate(zip(data, labels)):
			ps = self.model.predict(
				x=np.asarray([sequence]),
				batch_size=1,
				verbose=1
			)
			actual = 0
			if self.return_sequences:
				p = ps[0, -1]
				actual = label[-1]
			else:
				p = ps[0, 0]
				actual = label
			if plot:
				plt.figure(1)
				plt.scatter(i, actual, c='r')
				plt.scatter(i, p, c='b', alpha=0.6)
				plt.pause(0.02)
			if isinstance(actual, list):
				actual = actual[0]
			if isinstance(p, list):
				p = p[0]

			error = abs(actual - p)
			if actual < -2.4:
				bottom_errors.append(error)
			if -2.4 <= actual < 2.4:
				middle_errors.append(error)
			if actual > 2.4:
				top_errors.append(error)

		pickle.dump(bottom_errors, open('bottom_errors.p', 'wb'))
		pickle.dump(middle_errors, open('middle_errors.p', 'wb'))
		pickle.dump(top_errors, open('top_errors.p', 'wb'))
		max_error = max(bottom_errors + middle_errors + top_errors)
		plt.ion()
		plt.figure(2)
		plt.subplot(311)
		plt.title('Bottom Errors, p < ' + str(-2.4))
		plt.hist(
			x=bottom_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.subplot(312)
		plt.title('Middle Errors, p between ' + str(-2.4) + ' and ' + str(2.4))
		plt.hist(
			x=middle_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.subplot(313)
		plt.title('Top Errors, p > ' + str(2.4))
		plt.hist(
			x=top_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.waitforbuttonpress()
		print 'Average error: ' + str(np.mean(bottom_errors + middle_errors + top_errors))
		plt.waitforbuttonpress()


class Keras_LSTM_Slope(Keras_Model):

	def __init__(self, layers, num_features, max_seq_length=30, return_sequences=True, save_substr='K-SIMB_Slope'):
		Keras_Model.__init__(self, num_features, save_substr) # Super class
		self.return_sequences = return_sequences
		self.max_seq_length = max_seq_length

		if not self.loaded:
			self.model = Sequential()

			self.model.add(Masking(mask_value=0., input_shape=(max_seq_length, num_features)))

			for i, layer in enumerate(layers[:-1]):
				if i == 0:
					self.model.add(LSTM(
						input_dim=layers[0],
						output_dim=layers[1],
						return_sequences=True))
					self.model.add(Dropout(0.2))
					print "LSTM layer 1: "
					print(self.model.layers[-1].output_shape)

				if i > 1:
					return_full = return_sequences if i == len(layers[:-1]) - 1 else True
					self.model.add(LSTM(
						layer,
						return_sequences=return_full))
					self.model.add(Dropout(0.2))

					print "LSTM layer " + str(i) + ":"
					print(self.model.layers[-1].output_shape)

			if return_sequences:
				self.model.add(
					TimeDistributed(
						Dense(
							output_dim=layers[-1]
						)
					)
				)
			else:
				self.model.add(
					Dense(output_dim=1)
				)
			self.model.add(Activation("linear"))

			print "Output layer:"
			print(self.model.layers[-1].output_shape)

			start = time.time()
			self.model.compile(
				loss="mse",
				optimizer="rmsprop"
			)
			print "Compilation Time : ", time.time() - start

	def train_with_data(self, data, labels, batch_size, epochs):

		if not self.loaded:

			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=60
			)
			callbacks = [checkpoint, earlyStopping]
			data = np.asarray(data)
			labels = np.asarray(labels)

			self.model.fit(
				x=data,
				y=labels,
				batch_size=batch_size,
				nb_epoch=epochs,
				validation_split=0.05,
				callbacks=callbacks
			)

	def predict_on_data(self, data, labels, plot=True):
		bottom_errors = list()
		middle_errors = list()
		top_errors = list()
		plt.ion()
		for i, (sequence, label) in enumerate(zip(data, labels)):
			ps = self.model.predict(
				x=np.asarray([sequence]),
				batch_size=1,
				verbose=1
			)
			actual = 0
			if self.return_sequences:
				p = ps[0,-1]
				actual = label[-1]
			else:
				p = ps[0,0]
				actual = label
			plt.figure(1)
			plt.scatter(i, actual, c='r')
			plt.scatter(i, p, c='b', alpha=0.6)
			plt.pause(0.02)
			actual = actual[0]
			p = p[0]
			error = abs(actual - p)
			if actual < -2.4:
				bottom_errors.append(error)
			if -2.4 <= actual < 2.4:
				middle_errors.append(error)
			if actual > 2.4:
				top_errors.append(error)

		pickle.dump(bottom_errors, open('bottom_errors.p', 'wb'))
		pickle.dump(middle_errors, open('middle_errors.p', 'wb'))
		pickle.dump(top_errors, open('top_errors.p', 'wb'))
		max_error = max(bottom_errors + middle_errors + top_errors)
		plt.ion()
		plt.figure(2)
		plt.subplot(311)
		plt.title('Bottom Errors, p < ' + str(-2.4))
		plt.hist(
			x=bottom_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.subplot(312)
		plt.title('Middle Errors, p between ' + str(-2.4) + ' and ' + str(2.4))
		plt.hist(
			x=middle_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.subplot(313)
		plt.title('Top Errors, p > ' + str(2.4))
		plt.hist(
			x=top_errors,
			range=(0, max_error),
			bins=10,
			color='blue'
		)
		plt.waitforbuttonpress()
		print 'Average error: ' + str(np.mean(bottom_errors + middle_errors + top_errors))
		plt.waitforbuttonpress()


class Keras_DNN_Slope(Keras_Model):

	def __init__(self, layers, num_features, num_outputs, save_substr='K-SIMB_DNN_Slope'):

		Keras_Model.__init__(self, num_features, save_substr)  # Super class

		if not self.loaded:
			self.model = Sequential()
			for i, layer in enumerate(layers):
				if i == 0: # First layer
					self.model.add(
						Dense(
							output_dim=layer,
							init='glorot_uniform',
							activation='relu',
							input_dim=num_features
						)
					)
				else:
					self.model.add(
						Dense(
							output_dim=layer,
							init='glorot_uniform',
							activation='relu'
						)
					)
				self.model.add(
					Dropout(
						p=0.2
					)
				)
				print "Dense Layer with Relu:"
				print(self.model.layers[-1].output_shape)

			self.model.add(
				Dense(
					output_dim=num_outputs,
					init='glorot_uniform',
					activation='linear'
				)
			)
			print "Output layer:"
			print(self.model.layers[-1].output_shape)

			start = time.time()
			self.model.compile(
				loss="mse",
				optimizer="rmsprop"
			)
			print "Compilation Time : ", time.time() - start

	def train_with_data(self, data, labels, batch_size, epochs):
		Keras_Model.train_with_data(
			self,
			data=data,
			labels=labels,
			batch_size=batch_size,
			epochs=epochs)

	def predict_on_data(self, data, labels, plot=True):
		Keras_Model.predict_on_data(
			self,
			data=data,
			labels=labels,
			plot=plot
		)



if __name__ == '__main__':

	mode = 'Differences'
	#mode = 'Normalization'

	save_substr = ''

	try:
		sequences = list()
		labels = list()
		if mode == 'Differences':
			sequences = pickle.load(open('SIMB_sequences_diff.p', 'rb'))
			labels = pickle.load(open('SIMB_slopes_diff.p', 'rb'))
			save_substr = 'K_SIMB_DoN_Slope'
		if mode == 'Normalization':
			sequences = pickle.load(open('SIMB_sequences_normalized.p', 'rb'))
			labels = pickle.load(open('SIMB_slopes_normalized.p', 'rb'))
			save_substr = 'K_SIMB_N_Slope'
	except IOError:
		sequences, labels = DI.prepare_data(difference=True)

	everything = zip(sequences, labels)

	shuffle(everything)

	sequences, labels = zip(*everything)

	train_x = sequences[: int(0.9 * len(sequences))]
	train_y = labels[: int(0.9 * len(labels))]

	test_x = sequences[int(0.9 * len(sequences)):]
	test_y = labels[int(0.9 * len(labels)):]

	model = 'DNN'

	predictor = None

	if model == 'LSTM':

		num_features = np.shape(sequences)[-1]
		seq_length = np.shape(sequences)[-2]

		predictor = Keras_LSTM_Slope(
			layers=[1, 128, 128, 1],
			num_features=num_features,
			max_seq_length=seq_length,
			return_sequences=True,
			save_substr=save_substr
		)
	elif model == 'DNN':
		# Flatten the data
		train_x = [item for sublist in train_x for item in sublist]
		train_y = [item for sublist in train_y for item in sublist]

		test_x = [item for sublist in test_x for item in sublist]
		test_y = [item for sublist in test_y for item in sublist]

		predictor = Keras_DNN_Slope(
			layers=[64,64,64],
			num_features=np.shape(train_x)[-1],
			num_outputs=1,
			save_substr='K-SIMB_DNN_DoN'
		)
		shuffle(test_x)
		shuffle(test_y)

	predictor.train_with_data(
		data=train_x,
		labels=train_y,
		batch_size=32,
		epochs=50
	)

	predictor.predict_on_data(
		data=test_x,
		labels=test_y,
		plot=False
	)