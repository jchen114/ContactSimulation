import os
from keras.models import load_model
from keras.callbacks import ModelCheckpoint, EarlyStopping
import numpy as np


class Base_Model:

	def __init__(self, num_features, max_seq_length, save_substr, dir):

		self.substr = save_substr
		self.num_features = num_features
		self.max_seq_length = max_seq_length

		files = [f for f in os.listdir('.') if os.path.isfile(f)]
		self.loaded = False
		for f in files:
			if f.startswith(save_substr):
				print(f)
				self.model = load_model(f)
				self.loaded = True
				break

	def train_on_generator(self, data_generator, valid_generator, samples_per_epoch=1500, nb_epoch=50, continue_training=True):
		if continue_training:
			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=20
			)
			callbacks = [checkpoint, earlyStopping]

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