import time
from numpy import newaxis
from keras.layers.core import Dense, Activation, Dropout, Masking, Reshape, TimeDistributedDense, Merge
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

import DatabaseInterface as DI

import math
import numpy as np
from random import shuffle
import cPickle as pickle



class Slope_Ground_Predictor:

	def __init__(self, lstm_layers, slope_dense_layers, ground_dense_layers, state_num_features, force_num_features, max_seq_length):

		state_input = Masking(mask_value=0., input_shape=(max_seq_length, state_num_features))

		print 'state inputs: '
		print state_input.output_shape

		forces_input = Masking(mask_value=0, input_shape=(max_seq_length, force_num_features))
		print 'forces_inputs: '
		print forces_input.output_shape

		# Merge Layer
		X = Merge(
			layers=[state_input, forces_input],
			mode='concat'
		)

		print 'After Merge: '
		print X.output_shape

		# LSTM Layers
		for layer in lstm_layers:
			X = LSTM(
				output_dim=layer,
				return_sequences=True
			)(X)
			X = Dropout(
				p=0.2
			)(X)
			print 'After LSTM: '
			print X.output_shape

		# Output layers



def avg_down_foot_forces(forces, avg_window=4):
	avg_force_data = list()
	for force_sequence in forces:
		avg_force_sequence = list()
		for forces in force_sequence:
			avg_forces = list()
			for i in range(0, math.ceil(len(forces)/avg_window)):
				end_idx = i * avg_window + avg_window
				if end_idx > len(forces):
					end_idx = len(forces)
				avg_force = np.mean(forces[i*avg_window : end_idx])
				avg_forces.append(avg_force)
			avg_force_sequence.append(avg_forces)

		avg_force_data.append(avg_force_sequence)

if __name__ == '__main__':


	try:

		avg_forces = pickle.load(open('SIMB_forces_avg.p', 'rb'))
	except Exception:
		forces = pickle.load(open('SIMB_forces.p', 'rb'))
		avg_forces = avg_down_foot_forces(forces, avg_window=4)
		pickle.dump(avg_forces, open('SIMB_forces_avg.p', 'wb'))

	sequences_dat = pickle.load(open('SIMB_sequences_diff.p', 'rb'))
	labels_dat = pickle.load(open('SIMB_targets_diff.p', 'rb'))

	forces_dim = np.shape(avg_forces)[-1]
	state_dim = np.shape(sequences_dat)[-1]

	S_GC_predictor = Slope_Ground_Predictor(
		lstm_layers=[128,128],
		slope_dense_layers=[128],
		ground_dense_layers=[32, 32],
		state_num_features=state_dim,
		force_num_features=forces_dim,
		max_seq_length=30
	)
