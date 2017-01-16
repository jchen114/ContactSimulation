import lasagne
import theano.tensor as T
import theano
import numpy as np
import matplotlib.pyplot as plt

from random import shuffle

class LasagneLSTM:

	def __init__(self, hidden_units):
		lasagne.random.set_rng(np.random.RandomState(1))

		sequences = T.dtensor3('sequences')
		# targets = T.dvector('slope_targets')
		targets = T.dvector('targets')

		print "-- Lasagne: Building Network --"

		example = np.random.ranf(size=(1, 15, 1)).astype(dtype=np.float64)

		l_in = lasagne.layers.InputLayer(shape=(None, None, 1), input_var=sequences)

		print "after input: "
		print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

		hidden_units = [50, 100]

		i = 0

		ret_final = False
		# LSTM layers
		for units in hidden_units:
			if i == len(hidden_units) - 1:
				ret_final = True
			# Slope
			l_in = lasagne.layers.LSTMLayer(
				incoming=l_in,
				num_units=units,
				nonlinearity=lasagne.nonlinearities.tanh,
				only_return_final=ret_final)
			l_in = lasagne.layers.dropout(
				incoming=l_in,
				p=0.2
			)
			i += 1
			print "After LSTM: "
			print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

		if not ret_final:
			l_in = lasagne.layers.ReshapeLayer(l_in, (-1, hidden_units[-1]))
			print "After reshape: "
			print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

		# Output
		l_out = lasagne.layers.DenseLayer(
			incoming=l_in,
			num_units=1,
			nonlinearity=lasagne.nonlinearities.linear
		)

		print "After Dense: "
		print lasagne.layers.get_output(l_out).eval({sequences: example}).shape

		output = lasagne.layers.get_output(l_out)

		loss = 0.5 * lasagne.objectives.squared_error(output, targets).mean()

		params = lasagne.layers.get_all_params(l_out, trainable=True)

		updates = lasagne.updates.rmsprop(loss, params, learning_rate=0.1)

		self.seq_shared = theano.shared(
			np.zeros((1, 50, 1), dtype=np.float64)
		)

		self.targets_shared = theano.shared(
			np.zeros(1, dtype=np.float64)
		)

		# Train functions
		self._train = theano.function(
			inputs=[],
			outputs=[loss, output],
			updates=updates,
			givens={
				sequences: self.seq_shared,
				targets: self.targets_shared
			},
			allow_input_downcast=True
		)

		# Predict
		self._predict = theano.function(
			inputs=[],
			outputs=output,
			givens={
				sequences: self.seq_shared
			}
		)

	def train(self, train_dat, train_labels, batch_size, epochs):
		plt.ion()
		plt.figure(1)
		for epoch in range(0, epochs):
			completed_data = False
			batch_idx = 0
			indices = range(0, len(train_dat))
			while not completed_data:
				if batch_idx * batch_size + batch_size <= len(train_dat): # Still room for another batch
					batch = indices[batch_idx * batch_size : batch_idx * batch_size + batch_size]
				else:
					batch = indices[batch_idx * batch_size : ]
					completed_data = True
				batch_dats = [train_dat[i] for i in batch]
				batch_labels = [train_labels[i] for i in batch]
				self.seq_shared.set_value(batch_dats)
				self.targets_shared.set_value(batch_labels)
				batch_idx += 1
				cost, ps = self._train()
				plt.scatter(batch_idx, cost)
				plt.pause(0.05)
		plt.waitforbuttonpress()


	def predict_sequences_multiple(self, data, window_size, prediction_len):
		# Predict sequence of 50 steps before shifting prediction run forward by 50 steps
		prediction_seqs = []
		for i in xrange(len(data) / prediction_len):
			curr_frame = data[i * prediction_len]
			predicted = []
			for j in xrange(prediction_len):
				self.seq_shared.set_value([curr_frame])
				prediction = self._predict()[0, 0]
				predicted.append(prediction)
				curr_frame = curr_frame[1:]
				curr_frame = np.insert(curr_frame, [window_size - 1], predicted[-1], axis=0)
			prediction_seqs.append(predicted)
		return prediction_seqs
