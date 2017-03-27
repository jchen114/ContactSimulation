import os
from keras.models import load_model
from keras.callbacks import ModelCheckpoint, EarlyStopping, History, Callback
import numpy as np

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


class Histories(Callback):

	val_loss = []

	def __init__(self, dir_name):
		Callback.__init__(self)
		self.fname = dir_name + '/val_losses'

	def on_epoch_end(self, epoch, logs={}):
		print('Save val loss')
		self.val_loss.append(logs.get('val_loss'))

	def on_train_end(self, logs={}):
		# Save losses
		print('Save all losses')
		pickle.dump(self.val_loss, open(self.fname + '.p', 'wb'))

class Base_Model:

	def __init__(self, num_features, max_seq_length, save_substr, dir='.'):

		self.substr = save_substr
		self.num_features = num_features
		self.max_seq_length = max_seq_length
		self.dir = dir
		files = [f for f in os.listdir(dir) if os.path.isfile(dir + '/' + f)]
		self.loaded = False
		for f in files:
			if f.startswith(save_substr):
				print(f)
				self.model = load_model(dir+'/'+ f)
				self.loaded = True
				break

		self.substr = dir + '/' + save_substr

	def train_on_generator(self, data_generator, valid_generator, samples_per_epoch=1500, nb_epoch=50, continue_training=True):
		if continue_training:
			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=20
			)

			history = Histories('history')

			callbacks = [checkpoint, earlyStopping, history]

			self.model.fit_generator(
				generator=data_generator,
				samples_per_epoch=samples_per_epoch,
				nb_epoch=nb_epoch,
				verbose=1,
				validation_data=valid_generator,
				nb_val_samples=100,
				callbacks=callbacks
			)

	def train_on_data(self, data, labels, batch_size, epochs):
		filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
		checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
		earlyStopping = EarlyStopping(
			monitor='val_loss',
			patience=20
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

	def train_on_generator_validation_set(self, continue_training, data_gen, samples_per_epoch, nb_epoch, valid_data):
		if continue_training:
			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=20
			)

			history = Histories('history')

			callbacks = [checkpoint, earlyStopping, history]

			self.model.fit_generator(
				generator=data_gen,
				samples_per_epoch=samples_per_epoch,
				nb_epoch=nb_epoch,
				verbose=1,
				validation_data=valid_data,
				callbacks=callbacks
			)