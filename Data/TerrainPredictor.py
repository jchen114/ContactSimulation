import lasagne
import theano.tensor as T
import numpy as np
import cPickle as pickle
import theano
import matplotlib.pyplot as plt

import DatabaseInterface as DI

from random import shuffle


class Predictor():

    def __init__(self, hidden_units, num_features=16, num_outputs=3, batch_size=1, learning_rate=0.1, p_dropout=0.1):

        lasagne.random.set_rng(np.random.RandomState(1))

        sequences = T.dtensor3('sequences')
        slope_targets = T.dvector('slope_targets')
        stiffness_targets = T.dvector('stiffness_targets')
        damping_targets = T.dvector('damping_targets')

        print "-- Building Network --"

        example = np.random.ranf(size=(1, 50, 16)).astype(dtype=np.float64)

        l_in_slope = lasagne.layers.InputLayer(shape=(None, None, num_features), input_var=sequences)
        l_in_stiffness = lasagne.layers.InputLayer(shape=(None, None, num_features), input_var=sequences)
        l_in_damping = lasagne.layers.InputLayer(shape=(None, None, num_features), input_var=sequences)


        print "after input: "
        print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        # LSTM layers
        for units in hidden_units:
            # Slope
            l_lstm_slope = lasagne.layers.LSTMLayer(
                incoming=l_in_slope,
                num_units=units,
                nonlinearity=lasagne.nonlinearities.tanh,
                only_return_final=False)
            l_do_slope = lasagne.layers.DropoutLayer(l_lstm_slope, p=p_dropout)
            print "After LSTM + Dropout: "
            print lasagne.layers.get_output(l_do_slope).eval({sequences: example}).shape
            l_in_slope = l_do_slope

            # Stiffness
            l_lstm_stiffness = lasagne.layers.LSTMLayer(
                incoming=l_in_stiffness,
                num_units=units,
                nonlinearity=lasagne.nonlinearities.tanh,
                only_return_final=False)
            l_do_stiffness = lasagne.layers.DropoutLayer(l_lstm_stiffness, p=p_dropout)
            l_in_stiffness = l_do_stiffness

            # Damping
            l_lstm_damping = lasagne.layers.LSTMLayer(
                incoming=l_in_damping,
                num_units=units,
                nonlinearity=lasagne.nonlinearities.tanh,
                only_return_final=False)
            l_do_damping = lasagne.layers.DropoutLayer(l_lstm_damping, p=p_dropout)
            l_in_damping = l_do_damping

        # Reshape
        l_in_slope = lasagne.layers.ReshapeLayer(l_in_slope, (-1, hidden_units[-1]))
        l_in_stiffness = lasagne.layers.ReshapeLayer(l_in_stiffness, (-1, hidden_units[-1]))
        l_in_damping = lasagne.layers.ReshapeLayer(l_in_damping, (-1, hidden_units[-1]))
        print "After reshape: "
        print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        # # Regression
        # l_dense_slope = lasagne.layers.DenseLayer(
        #     incoming=l_in_slope,
        #     num_units=32,
        #     W=lasagne.init.Normal(),
        #     nonlinearity=lasagne.nonlinearities.rectify
        # )
        #
        # l_dense_stiffness = lasagne.layers.DenseLayer(
        #     incoming=l_in_stiffness,
        #     num_units=32,
        #     W=lasagne.init.Normal(),
        #     nonlinearity=lasagne.nonlinearities.rectify
        # )
        #
        # l_dense_damping = lasagne.layers.DenseLayer(
        #     incoming=l_in_damping,
        #     num_units=32,
        #     W=lasagne.init.Normal(),
        #     nonlinearity=lasagne.nonlinearities.rectify
        # )

        # Output
        self.l_out_slope = lasagne.layers.DenseLayer(
            incoming=l_in_slope,
            num_units=1,
            W=lasagne.init.Normal(),
            nonlinearity=lasagne.nonlinearities.linear
        )

        self.l_out_stiffness = lasagne.layers.DenseLayer(
            incoming=l_in_stiffness,
            num_units=1,
            W=lasagne.init.Normal(),
            nonlinearity=lasagne.nonlinearities.linear
        )

        self.l_out_damping = lasagne.layers.DenseLayer(
            incoming=l_in_damping,
            num_units=1,
            W=lasagne.init.Normal(),
            nonlinearity=lasagne.nonlinearities.linear
        )

        self.slope_output = lasagne.layers.get_output(self.l_out_slope)
        self.stiffness_output = lasagne.layers.get_output(self.l_out_stiffness)
        self.damping_output = lasagne.layers.get_output(self.l_out_damping)

        self.slope_cost = T.mean(0.5 * lasagne.objectives.squared_error(slope_targets, self.slope_output))
        self.stiffness_cost = T.mean(0.5 * lasagne.objectives.squared_error(stiffness_targets, self.stiffness_output))
        self.damping_cost = T.mean(0.5 * lasagne.objectives.squared_error(damping_targets, self.damping_output))

        self.slope_params = lasagne.layers.get_all_params(self.l_out_slope, trainable=True)
        self.stiffness_params = lasagne.layers.get_all_params(self.l_out_stiffness, trainable=True)
        self.damping_params = lasagne.layers.get_all_params(self.l_out_damping, trainable=True)

        updates_slope = lasagne.updates.rmsprop(self.slope_cost, self.slope_params, learning_rate)
        updates_stiffness = lasagne.updates.rmsprop(self.stiffness_cost, self.stiffness_params, learning_rate)
        updates_damping = lasagne.updates.rmsprop(self.damping_cost, self.damping_params, learning_rate)

        self._seq_shared = theano.shared(
            np.zeros((1, 50, num_features), dtype=np.float64)
        )

        self._slope_targets = theano.shared(
            np.zeros(50, dtype=np.float64)
        )

        self._stiffness_targets = theano.shared(
            np.zeros(50, dtype=np.float64)
        )

        self._damping_targets = theano.shared(
            np.zeros(50, dtype=np.float64)
        )

        self._predict_shared = theano.shared(
            np.zeros((1, 50, num_features), dtype=np.float64)
        )

        # Train functions
        self._train_slope = theano.function(
            inputs=[],
            outputs=[self.slope_cost, self.slope_output],
            updates=updates_slope,
            givens={
                sequences: self._seq_shared,
                slope_targets: self._slope_targets
            },
            allow_input_downcast=True
        )

        self._train_stiffness = theano.function(
            inputs=[],
            outputs=[self.stiffness_cost, self.stiffness_output],
            updates=updates_stiffness,
            givens={
                sequences: self._seq_shared,
                stiffness_targets: self._stiffness_targets
            },
            allow_input_downcast=True
        )

        self._train_damping = theano.function(
            inputs=[],
            outputs=[self.damping_cost, self.damping_output],
            updates=updates_damping,
            givens={
                sequences: self._seq_shared,
                damping_targets: self._damping_targets
            },
            allow_input_downcast=True
        )

        # Predict

        self._predict_slope = theano.function(
            inputs=[],
            outputs=self.slope_output,
            givens={
                sequences: self._predict_shared
            }
        )

        self._predict_stiffness = theano.function(
            inputs=[],
            outputs=self.stiffness_output,
            givens={
                sequences: self._predict_shared
            }
        )

        self._predict_damping = theano.function(
            inputs=[],
            outputs=self.damping_output,
            givens={
                sequences: self._predict_shared
            }
        )

    def save_model(self):
        np.savez('model_slope.npz', *lasagne.layers.get_all_param_values(self.l_out_slope))
        np.savez('model_damping.npz', *lasagne.layers.get_all_param_values(self.l_out_damping))
        np.savez('model_stiffness.npz', *lasagne.layers.get_all_param_values(self.l_out_stiffness))

    def load_model(self):
        try:
            with np.load('model_slope.npz') as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]
                lasagne.layers.set_all_param_values(self.l_out_slope, param_values)
            with np.load('model_damping.npz') as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]
                lasagne.layers.set_all_param_values(self.l_out_damping, param_values)
            with np.load('model_stiffness.npz') as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]
                lasagne.layers.set_all_param_values(self.l_out_stiffness, param_values)
            return True
        except IOError as e:
            return False

    def train(self, train_data, train_labels, epochs=10):
        plt.ion()
        i = 0
        plt.figure(1)
        plt.subplot(311)
        plt.title('Slope')
        plt.subplot(312)
        plt.title('Stiffness')
        plt.subplot(313)
        plt.title('Damping')
        # Training
        for epoch in range (0, epochs):
            for train, labels in zip(train_data, train_labels):
                self._seq_shared.set_value([train])

                slope_labels = [label[0] for label in labels]
                stiffness_labels = [label[1] for label in labels]
                damping_labels = [label[2] for label in labels]

                self._slope_targets.set_value(slope_labels)
                self._stiffness_targets.set_value(stiffness_labels)
                self._damping_targets.set_value(damping_labels)

                cost_slope, _ = self._train_slope()
                cost_stiffness, _ = self._train_stiffness()
                cost_damping, _ = self._train_damping()
                plt.subplot(311)
                plt.scatter(i, cost_slope)
                plt.subplot(312)
                plt.scatter(i, cost_stiffness)
                plt.subplot(313)
                plt.scatter(i, cost_damping)
                i += 1
                plt.pause(0.05)
        plt.waitforbuttonpress()

    def validate_model(self, valid_data, valid_labels ):
        # Validation
        plt.ion()
        p_idx = 0
        plt.figure(2)
        plt.subplot(311)
        plt.title('Slope')
        plt.subplot(312)
        plt.title('Stiffness')
        plt.subplot(313)
        plt.title('Damping')
        for valid, labels in zip(valid_data, valid_labels):
            self._predict_shared.set_value([valid])
            slope_predictions = self._predict_slope()
            stiffness_predictions = self._predict_stiffness()
            damping_predictions = self._predict_damping()
            slope_labels = [label[0] for label in labels]
            stiffness_labels = [label[1] for label in labels]
            damping_labels = [label[2] for label in labels]
            for pt in range(0, len(labels)):
                plt.subplot(311)
                plt.scatter(p_idx, slope_predictions[pt][0], c='b')
                plt.scatter(p_idx, slope_labels[pt], c='r')
                plt.subplot(312)
                plt.scatter(p_idx, stiffness_predictions[pt][0], c='b')
                plt.scatter(p_idx, stiffness_labels[pt], c='r')
                plt.subplot(313)
                plt.scatter(p_idx, damping_predictions[pt][0], c='b')
                plt.scatter(p_idx, damping_labels[pt], c='r')
                p_idx += 1
                plt.pause(0.05)
        plt.waitforbuttonpress()

if __name__ == '__main__':

    predictor = Predictor([100])

    data = list()
    labels = list()
    try:
        data = pickle.load(open('data.p', 'rb'))
        labels = pickle.load(open('labels.p', 'rb'))
    except Exception as e:
        db_conn = DI.initialize_db_connection('samples.db')
        db_conn.row_factory = DI.dict_factory
        num_sequences = DI.get_number_of_sequences(db_conn)['MAX(SEQUENCE_ID)']
        data = list()
        labels = list()
        for trial in range(1, num_sequences + 1):
            state_ids = DI.get_sequence(db_conn, trial)
            sequences, targets = DI.get_states_from_state_ids(db_conn, state_ids)
            data.extend(sequences)
            labels.extend(targets)
        # Split data into train, validation and test
        pickle.dump(data, open('data.p', 'wb'))
        pickle.dump(labels, open('labels.p', 'wb'))
    z_data = zip(data, labels)
    shuffle(z_data)
    data, labels = zip(*z_data)
    train_data = data[:int(0.7*len(data))]
    train_labels = labels[:int(0.7*len(labels))]
    valid_data = data[int(0.7*len(data)) + 1 : int(0.9*len(data))]
    valid_labels = labels[int(0.7*len(labels)) + 1 : int(0.9*len(labels))]
    test_data = data[int(0.9 * len(data)) + 1 :]
    test_labels = labels[int(0.9*len(labels)) + 1 :]
    if not predictor.load_model():
        predictor.train(train_data, train_labels)
        predictor.save_model()
    predictor.validate_model(valid_data=valid_data, valid_labels=valid_labels)





