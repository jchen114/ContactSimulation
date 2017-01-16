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

import math
import numpy as np

import cPickle as pickle


def sample_slope(min, max):
	slope = np.random.uniform(low=min, high=max)
	return slope


def get_positions_of_wheel(angular_velocity, slope, time, num_points=4, radius=1, start_x=0.0, start_y=0.0, start_rotation=0.0):

	# Assume that the wheel has already moved 2*pi*r into the hill for simplicity..
	theta_slope = math.atan(slope)

	# The center at the beginning of the experiment
	#Bx = -radius * math.sin(theta_slope) + start_x
	#By = radius * math.cos(theta_slope) + start_y
	Bx = start_x
	By = start_y

	# Get the starting positions of the wheel.
	# Construct the position matrix
	positions = np.linspace(
		start=0,
		stop=2 * math.pi,
		num=num_points,
		endpoint=False
	)
	positions = [(radius * math.cos(position), radius * math.sin(position)) for position in positions]

	C = np.ones(shape=(2, len(positions)))
	for i in range(0, len(positions)):
		C[0, i] = positions[i][0]
		C[1, i] = positions[i][1]

	# angular velocity in radians, positive = forwards, negative = backwards

	# Build Rotation Matrix
	displacement = math.fmod(angular_velocity * time, 2 * math.pi) # Get how much the wheel has rotated by
	displacement = -displacement  # accounting for the fact that positive displacement means that the wheel rotated CCW
	rotation = math.fmod(displacement + start_rotation + theta_slope, 2 * math.pi)
	R = np.zeros(
		shape=(2, 2)
	)
	R[0, 0] = math.cos(rotation)
	R[0, 1] = - math.sin(rotation)
	R[1, 0] = math.sin(rotation)
	R[1, 1] = math.cos(rotation)

	rotated_points = np.dot(R, C)

	# Translate the points to where the center is..
	# Build Translation Matrix

	dist_traveled = angular_velocity * time * radius
	dist_x = dist_traveled * math.cos(theta_slope) + Bx
	dist_y = dist_traveled * math.sin(theta_slope) + By

	T = np.zeros(shape=(3, 3))
	np.fill_diagonal(T, 1)

	T[0, 2] = dist_x
	T[1, 2] = dist_y

	ones_row = [1] * len(positions)

	rotated_points = np.vstack([rotated_points, ones_row])

	final_positions = np.dot(T, rotated_points)
	final_positions = np.delete(final_positions, 2, 0)

	fps = list()

	for col in range(0, final_positions.shape[1]):
		final_position = final_positions[:, col]
		p = final_position[0], final_position[1]
		fps.append(p)
	return fps, dist_x, dist_y, rotation


def plot_wheel_points(points):
	plt.ion()
	plt.axis('equal')
	#`plt.margins(x=1, y=1)
	#plt.locator_params(tight=True, nbins=5)
	cs = [[1.0/len(points[0])] * 3] * len(points[0])
	for i, c in enumerate(cs):
		cs[i] = map(lambda x: x * i, c)
		cs[i][i%3] = 0.0
	#plt.axis([0, 35, 0, 35])
	for ps in points:
		#x,y = ps[0]
		#plt.scatter(x, y, c= 'b')
		#plt.pause(0.05)
		for i, (x,y) in enumerate(ps):
			plt.scatter(x,y, c=cs[i])
			plt.pause(0.05)
	plt.waitforbuttonpress()


def test_wheel_positions():
	ang_vel = math.pi/4
	points = list()
	x, y, disp = 0.0, 0.0, 0.0
	for t in np.arange(0, 20, 5.5):
		pts, x, y, disp = get_positions_of_wheel(
			angular_velocity=ang_vel,
			slope=math.pi/6,
			time=t,
			num_points=10,
			radius=1,
			start_x=5.0,
			start_y=5.0
		)
		points.append(pts)

	for t in np.arange(0, 20, 5.5):

		pts, _, _, _ = get_positions_of_wheel(
			angular_velocity=ang_vel,
			slope=0.0,
			time=t,
			num_points=10,
			radius=1,
			start_x=x,
			start_y=y,
			start_rotation=disp
		)

		points.append(pts)

	plot_wheel_points(points)


def wheel_sequence_generator(angular_velocity, num_points=4, radius=1, min_slope=0.0, max_slope=2.0):

	while True:
		# sample duration
		duration = np.random.uniform(low=15, high=30)
		# sample slope
		slope = sample_slope(min_slope, max_slope)
		positions = list()
		for t in np.arange(start=0.0, stop=duration, step=0.5):
			poss = get_positions_of_wheel(
				angular_velocity=angular_velocity,
				slope=slope,
				time=t,
				num_points=num_points,
				radius=radius
			)
			positions.append(poss)
		yield positions, slope


def terrain_generator(num_slopes, angular_velocity, num_points=4, radius=1):
	old_positions = (0, 0)
	start_rotation = 0
	wheel_positions = list()
	slopes = list()
	for section in range(0, num_slopes):
		slope = sample_slope(-2, 2)
		duration = np.random.uniform(low=22.5, high=36)
		x, y, final_rotation = 0.0, 0.0, 0.0
		for t in np.arange(start=0.0, stop=duration, step=4.5):
			positions, x, y, rotation = get_positions_of_wheel(
				angular_velocity=angular_velocity,
				slope=slope,
				time=t,
				num_points=num_points,
				radius=radius,
				start_x=old_positions[0],
				start_y=old_positions[1],
				start_rotation=start_rotation
			)
			final_rotation = rotation
			if section > 0:
				if t > 0.0:
					wheel_positions.append(positions)
					slopes.append(slope)
			else:
				wheel_positions.append(positions)
				slopes.append(slope)
		start_rotation = final_rotation # Save how much the wheel has turned
		old_positions = (x, y)
	return wheel_positions, slopes


def prepare_data(wheel_positions, slopes, seq_length, full_sequence=True, data_processing='Normalize'):
	data = list()
	labels = list()
	for i in range(0, len(wheel_positions) - seq_length + 1):
		sequence = wheel_positions[i:i+seq_length]
		y_positions = list()
		for element in sequence:
			pos = [y for (x,y) in element]
			y_positions.append(pos)
		labels_to_add = slopes[i+seq_length-1]
		if data_processing == 'Normalize':
			y_positions = normalize_sequence(y_positions)
			if full_sequence:
				labels_to_add = [[slope] for slope in slopes[i:i+seq_length]]
		if data_processing == 'Difference':
			y_positions = difference_sequence(y_positions)
			if full_sequence:
				labels_to_add = [[slope] for slope in slopes[i+1:i+seq_length]]
		labels.append(labels_to_add)
		data.append(y_positions)
	return data, labels


def normalize_sequence(sequence):
	new_sequence = list()
	first_element = sequence[0]
	for element in sequence:
		transformed_data_pt = [x/y-1 for (x,y) in zip(element, first_element)]
		new_sequence.append(transformed_data_pt)
	return new_sequence


def difference_sequence(sequence):
	diff_seq = list()
	init_dat = sequence[0]
	for dat in sequence[1:]:
		seq = [y-x for x,y in zip(init_dat, dat)]
		diff_seq.append(seq)
		init_dat = dat
	return diff_seq


def save_data(data, labels, file_name):
	pickle.dump(
		{
			'wheel_positions': data,
			'slopes': labels
		},
		open(file_name + '.p', 'wb')
	)


def load_data(file_name):
	try:
		stuff = pickle.load(open(file_name + '.p', 'rb'))
		return stuff['wheel_positions'], stuff['slopes']
	except IOError:
		raise IOError


class wheel_slope_predictor:

	def __init__(self, layers, max_seq_length=20, num_features=1, return_sequences=False, save_substr='K-Terrains-VSeq'):

		self.return_sequences = return_sequences
		self.max_seq_length = max_seq_length
		self.num_features = num_features
		self.loaded = False
		self.substr = save_substr
		files = [f for f in os.listdir('.') if os.path.isfile(f)]
		for f in files:
			if f.startswith(save_substr):
				print f
				self.model = load_model(f)
				self.loaded = True
				break

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

	def train(self, generator, batch_size, epochs):
		if not self.loaded:
			dat = list()
			dat_lbl = list()
			for b in range(0, batch_size * 100):  # Build the batch
				sequence = [[0] * self.num_features] * self.max_seq_length
				seq, slope = next(generator)
				for i, positions in enumerate(seq):
					# list of tuples
					y_pos = list()
					for (x,y) in positions:
						y_pos.append(y)
					sequence[i] = y_pos
				if not self.return_sequences:
					label = slope
				else:
					label = [[slope]] * self.max_seq_length

				dat.append(sequence)
				dat_lbl.append(label)
			batch = np.asarray(dat)
			filepath = self.substr + "-{epoch:02d}-{val_loss:.5f}.hdf5"
			checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, mode='auto')
			earlyStopping = EarlyStopping(
				monitor='val_loss',
				patience=10
			)
			callbacks = [checkpoint, earlyStopping]
			# batch_labels = np.expand_dims(dat_lbl, -1)
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

	def predict(self, generator, num_predictions):
		plt.ion()
		for prediction in range(0, num_predictions):
			sequence = [[0] * self.num_features] * self.max_seq_length
			seq, slope = next(generator)
			for i, positions in enumerate(seq):
				# list of tuples
				y_pos = list()
				for (x, y) in positions:
					y_pos.append(y)
				sequence[i] = y_pos
			ps = self.model.predict(
				x=np.asarray([sequence]),
				batch_size=1
			)
			if self.return_sequences:
				p = ps[0, -1]
			else:
				p = ps[0, 0]
			plt.scatter(prediction, slope, c='r')
			plt.scatter(prediction, p, c='b')
			plt.pause(0.05)
		plt.waitforbuttonpress()

	def predict_on_data(self, data, labels):
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
			plt.scatter(i, actual, c='r')
			plt.scatter(i, p, c='b')
			plt.pause(0.05)
		plt.waitforbuttonpress()

if __name__ == '__main__':

	num_positions_wheel = 10
	max_seq_length = 7
	full_sequence = True
	data_processing = 'Difference'

	#test_wheel_positions()
	try:
		wheel_positions, slopes = load_data('wheel_train_data')
		plot_wheel_points(wheel_positions[:3])
	except IOError:

		wheel_positions, slopes = terrain_generator(
			num_slopes=3000,
			angular_velocity=math.pi/6,
			num_points=num_positions_wheel,
			radius=1
		)

		save_data(wheel_positions, slopes, 'wheel_train_data')

	data, labels = prepare_data(
		wheel_positions=wheel_positions,
		slopes=slopes,
		seq_length=max_seq_length,
		full_sequence=full_sequence,
		data_processing=data_processing
	)

	if full_sequence:
		save_substr = 'K-Terrains-FullSeq'
	else:
		save_substr = 'K-Terrains-RetFinal'

	seq_length = max_seq_length

	if data_processing == 'Difference':
		seq_length = max_seq_length - 1

	slope_wheel_model = wheel_slope_predictor(
		layers=[1, 128, 128, 1],
		max_seq_length=seq_length,
		num_features=num_positions_wheel,
		return_sequences=full_sequence,
		save_substr=save_substr
	)

	slope_wheel_model.train_with_data(
		data=data,
		labels=labels,
		batch_size=32,
		epochs=50
	)

	# Make sure its working..
	slope_wheel_model.predict_on_data(data[:50], labels[:50])

	try:
		test_wheel_positions, test_slopes = load_data('wheel_test_data')
	except:
		test_wheel_positions, test_slopes = terrain_generator(
			num_slopes=25,
			angular_velocity=math.pi/6,
			num_points=num_positions_wheel,
			radius=1
		)
		save_data(test_wheel_positions, test_slopes, 'wheel_test_data')

	test_wheel_positions, test_slopes = prepare_data(
		wheel_positions=test_wheel_positions,
		slopes=test_slopes,
		seq_length=max_seq_length,
		full_sequence=full_sequence,
		data_processing=data_processing
	)

	slope_wheel_model.predict_on_data(test_wheel_positions, test_slopes)