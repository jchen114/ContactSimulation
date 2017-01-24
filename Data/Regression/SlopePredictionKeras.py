import time
import warnings
import numpy as np
from numpy import newaxis
from keras.layers.core import Dense, Activation, Dropout, Masking
from keras.layers.recurrent import LSTM
from keras.callbacks import ModelCheckpoint
from keras.models import Sequential
import matplotlib.pyplot as plt
from keras.utils.np_utils import to_categorical


def slope_generator(step_size, min_steps, max_steps, low=-5, high=5, discrete=True):
	while True:
		if min_steps == max_steps:
			num_steps = min_steps
		else:
			num_steps = np.random.randint(min_steps, max_steps)
		xs = step_size * np.arange(0, num_steps)
		if discrete:
			start = np.random.randint(low=-5, high=5)
			slope = np.random.choice([-2, -1, 0, 1, 2], p=[0.2, 0.2, 0.2, 0.2, 0.2])
		else:
			start = np.random.uniform(low=low, high=high)
			slope = np.random.uniform(low=-3, high=3) # Continous slope
		y = [[start + slope*x] for x in xs]
		yield y, slope

class LSTM_Keras:

	def __init__(self, layers, discrete=True, max_seq_length=20, num_features=1):
		self.model = Sequential()

		self.model.add(Masking(mask_value=0., input_shape=(max_seq_length, num_features)))

		self.model.add(LSTM(
			input_dim=layers[0],
			output_dim=layers[1],
			return_sequences=True))
		self.model.add(Dropout(0.2))

		self.model.add(LSTM(
			layers[2],
			return_sequences=False))
		self.model.add(Dropout(0.2))

		self.discrete = discrete

		if discrete:
			self.model.add(Dense(
				output_dim=layers[3]))
			self.model.add(Activation("softmax"))

			start = time.time()
			self.model.compile(
				loss="categorical_crossentropy",
				optimizer="rmsprop"
			)
		else:
			self.model.add(
				Dense(output_dim=1)
			)
			self.model.add(Activation("linear"))

			start = time.time()
			self.model.compile(
				loss="mse",
				optimizer="rmsprop"
			)
		print "Compilation Time : ", time.time() - start

	def train_in_batches(self, generator, batch_size, epochs, max_seq_length):
		dat = list()
		dat_lbl = list()
		for b in range(0, batch_size * 100): # Build the batch
			sequence = [[0]] * max_seq_length
			seq, slope = next(generator)
			sequence[0:len(seq)] = seq
			if self.discrete:
				label = slope + 2
			else:
				label = slope
			dat.append(sequence)
			#labels = [0] * 5
			#labels[label] = 1
			dat_lbl.append(label)
		batch = np.asarray(dat)
		if discrete:
			batch_labels = to_categorical(dat_lbl, nb_classes=None)

		filepath = "Keras-Slope-VarSeq-{epoch:02d}-{val_loss:.2f}.hdf5"
		checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
		callbacks = [checkpoint]
		#batch_labels = np.expand_dims(dat_lbl, -1)
		batch_labels = np.asarray(dat_lbl)
		self.model.fit(
			x=batch,
			y=batch_labels,
			batch_size=batch_size,
			nb_epoch=epochs,
			verbose=1,
			validation_split=0.05,
			callbacks=callbacks
		)

	def predict(self, generator, num_predictions):
		plt.ion()
		for prediction in range (0, num_predictions):
			sequence = [[0]] * 20
			seq, slope = next(generator)
			sequence[:len(seq)] = seq
			ps = self.model.predict(
				x=np.asarray([sequence]),
				batch_size=1
			)
			if self.discrete:
				p = np.argmax(ps[0])
				plt.scatter(prediction, slope + 2, c='r')
			else:
				p = ps[0,0]
				plt.scatter(prediction, slope, c='r')
			plt.scatter(prediction, p, c='b')
			plt.pause(0.05)
		plt.waitforbuttonpress()


if __name__ == '__main__':

	discrete = False
	# Build the training data..
	slope_gen = slope_generator(
		step_size=0.02,
		min_steps=10,
		max_steps=20,
		low=-5,
		high=3,
		discrete=discrete
	)

	model = LSTM_Keras([1, 100, 100, 1], discrete=discrete)
	model.train_in_batches(
		generator=slope_gen,
		batch_size = 30,
		epochs=100,
		max_seq_length=20
	)

	valid_gen = slope_generator(
		step_size=0.02,
		min_steps=10,
		max_steps=20,
		low=-5,
		high=3,
		discrete=discrete
	)

	model.predict(generator=valid_gen, num_predictions=50)
