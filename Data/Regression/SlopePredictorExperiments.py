import lasagne
import theano.tensor as T
import numpy as np
import cPickle as pickle
import theano
import matplotlib.pyplot as plt

import DatabaseInterface as DI

from random import shuffle

class PredictorLSTM:

    def __init__(self,
                 hidden_units,
                 num_features=16,
                 num_outputs=3,
                 batch_size=1,
                 learning_rate=0.8,
                 p_dropout=0.1,
                 return_final=True):

        lasagne.random.set_rng(np.random.RandomState(1))

        sequences = T.dtensor3('sequences')
        slope_targets = T.dvector('slope_targets')

        print "-- Building Network --"

        example = np.random.ranf(size=(1, 50, 16)).astype(dtype=np.float64)

        l_in_slope = lasagne.layers.InputLayer(shape=(None, None, num_features), input_var=sequences)


        print "after input: "
        print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        # LSTM layers
        for units in hidden_units:
            # Slope
            l_in_slope = lasagne.layers.RecurrentLayer(
                incoming=l_in_slope,
                num_units=units,
                nonlinearity=lasagne.nonlinearities.tanh,
                only_return_final=return_final)
            print "After LSTM:"
            print lasagne.layers.get_output(l_in_slope).eval({sequences:example}).shape

        # Reshape
        # l_in_slope = lasagne.layers.ReshapeLayer(l_in_slope, (-1, hidden_units[-1]))
        #
        # print "After reshape: "
        # print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        # l_in_slope = lasagne.layers.DenseLayer(
        #     incoming=l_in_slope,
        #     num_units=64,
        #     W=lasagne.init.Normal(),
        #     nonlinearity=lasagne.nonlinearities.rectify
        # )

        # # Regression
        # l_dense_slope = lasagne.layers.DenseLayer(
        #     incoming=l_in_slope,
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

        self.slope_output = lasagne.layers.get_output(self.l_out_slope)
        self.slope_cost = T.mean(0.5 * lasagne.objectives.squared_error(slope_targets, self.slope_output))

        self.slope_params = lasagne.layers.get_all_params(self.l_out_slope, trainable=True)

        updates_slope = lasagne.updates.rmsprop(self.slope_cost, self.slope_params, learning_rate=learning_rate)
        #updates_slope = lasagne.updates.apply_momentum(updates=updates_slope, params=self.slope_params, momentum=0.3)

        self._seq_shared = theano.shared(
            np.zeros((1, 50, num_features), dtype=np.float64)
        )

        self._slope_targets = theano.shared(
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

        # Predict

        self._predict_slope = theano.function(
            inputs=[],
            outputs=self.slope_output,
            givens={
                sequences: self._predict_shared
            }
        )

    def save_model(self):
        np.savez('model_slope.npz', *lasagne.layers.get_all_param_values(self.l_out_slope))

    def load_model(self):
        try:
            with np.load('model_slope.npz') as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]
                lasagne.layers.set_all_param_values(self.l_out_slope, param_values)
            return True
        except IOError as e:
            return False

    def train(self, train_data, train_labels, epochs=10):
        plt.ion()
        i = 0
        plt.figure(1)
        plt.title('Slope')

        # Training
        # for epoch in range (0, epochs):
        #     for train, labels in zip(train_data, train_labels):
        #         self._seq_shared.set_value([train])
        #         slope_labels = [label[0] for label in labels]
        #         self._slope_targets.set_value(slope_labels)
        #
        #         cost_slope, _ = self._train_slope()
        #
        #         i += 1
        #         plt.pause(0.05)
        # plt.waitforbuttonpress()

        # Overfit this one training sequence
        train_seq = [seq for seq in train_data[0]]
        train_label = [label[0] for label in train_labels[0]]
        seq_length = 15
        for epoch in range(0, epochs):
            for train in range(0, len(train_seq) - seq_length):
                seq = train_seq[train : train + seq_length]
                label = train_label[train + seq_length - 1]
                self._seq_shared.set_value([seq])
                self._slope_targets.set_value([label])
                cost, ps = self._train_slope()
                plt.scatter(train, cost)
                plt.pause(0.05)
        plt.waitforbuttonpress()

    def validate_model(self, valid_data, valid_labels ):
        # Validation
        plt.ion()
        p_idx = 0
        plt.figure(2)
        plt.title('Slope')
        for valid, labels in zip(valid_data, valid_labels):
            self._predict_shared.set_value([valid])
            slope_predictions = self._predict_slope()
            slope_labels = [label[0] for label in labels]
            stiffness_labels = [label[1] for label in labels]
            damping_labels = [label[2] for label in labels]
            for pt in range(0, len(labels)):
                plt.scatter(p_idx, slope_predictions[pt][0], c='b')
                plt.scatter(p_idx, slope_labels[pt], c='r')
                p_idx += 1
                plt.pause(0.05)
        plt.waitforbuttonpress()


class PredictorDNN:

    def __init__(self, hidden_units, num_features=16, batch_size=10, learning_rate=0.1):
        lasagne.random.set_rng(np.random.RandomState(1))
        sequences = T.matrix('sequences')
        slope_targets = T.dvector('slope_targets')

        print "-- Building Network --"

        example = np.random.ranf(size=(batch_size, 16)).astype(dtype=np.float64)

        l_in_slope = lasagne.layers.InputLayer(shape=(None, num_features), input_var=sequences)

        print "after input: "
        print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        for units in hidden_units:
            l_in_slope = lasagne.layers.DenseLayer(
                incoming=l_in_slope,
                num_units=units,
                nonlinearity=lasagne.nonlinearities.rectify
            )
            print "Dense Layer shape:"
            print lasagne.layers.get_output(l_in_slope).eval({sequences: example}).shape

        # Output
        self.l_out_slope = lasagne.layers.DenseLayer(
            incoming=l_in_slope,
            num_units=1,
            W=lasagne.init.Normal(),
            nonlinearity=lasagne.nonlinearities.linear
        )

        self.slope_output = lasagne.layers.get_output(self.l_out_slope)
        self.slope_cost = T.mean(0.5 * lasagne.objectives.squared_error(slope_targets, self.slope_output))

        self.slope_params = lasagne.layers.get_all_params(self.l_out_slope, trainable=True)

        updates_slope = lasagne.updates.rmsprop(self.slope_cost, self.slope_params, learning_rate=learning_rate)
        # updates_slope = lasagne.updates.apply_momentum(updates=updates_slope, params=self.slope_params, momentum=0.3)

        self._seq_shared = theano.shared(
            np.zeros((batch_size, num_features), dtype=np.float64)
        )

        self._slope_targets = theano.shared(
            np.zeros(batch_size, dtype=np.float64)
        )

        self._predict_shared = theano.shared(
            np.zeros((1, num_features), dtype=np.float64)
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

        # Predict
        self._predict_slope = theano.function(
            inputs=[],
            outputs=self.slope_output,
            givens={
                sequences: self._predict_shared
            }
        )

    def save_model(self):
        np.savez('model_slope_dnn.npz', *lasagne.layers.get_all_param_values(self.l_out_slope))

    def load_model(self):
        try:
            with np.load('model_slope_dnn.npz') as f:
                param_values = [f['arr_%d' % i] for i in range(len(f.files))]
                lasagne.layers.set_all_param_values(self.l_out_slope, param_values)
            return True
        except IOError as e:
            return False

    def train(self, train_data, train_labels, epochs=10):
        plt.ion()
        plt.figure(1)
        plt.subplot(211)
        plt.title('loss')
        plt.subplot(212)
        plt.title('predictions')

        # Training
        # for epoch in range (0, epochs):
        #     for train, labels in zip(train_data, train_labels):
        #         self._seq_shared.set_value([train])
        #         slope_labels = [label[0] for label in labels]
        #         self._slope_targets.set_value(slope_labels)
        #
        #         cost_slope, _ = self._train_slope()
        #
        #         i += 1
        #         plt.pause(0.05)
        # plt.waitforbuttonpress()

        # Overfit this one training sequence
        train_seq = [seq for seq in train_data[0]]
        train_label = [label[0] for label in train_labels[0]]
        batch_size = 10

        trial = 0
        p_i = 0

        print "mean of labels: " + str(np.mean(train_label))

        for epoch in range(0, epochs):
            for train in range(0, 50):
                # Grab a batch of 10?
                indices = range(0, len(train_seq))
                shuffle(indices)
                batch = indices[:batch_size]
                samples = [train_seq[i] for i in batch]
                labels = [train_label[i] for i in batch]
                # Train the batch of 10.
                self._seq_shared.set_value(samples)
                self._slope_targets.set_value(labels)
                cost_slope, ps = self._train_slope()
                trial += 1
                plt.subplot(211)
                plt.scatter(trial, cost_slope)
                plt.pause(0.05)
                plt.subplot(212)
                for p,l in zip(ps, labels):
                    plt.scatter(p_i, p, c='b')
                    plt.scatter(p_i, l, c='r')
                    p_i += 1
                    plt.pause(0.05)
        plt.waitforbuttonpress()

        #self.save_model()

        #self.load_model()

        indices = range(0, len(train_seq))
        shuffle(indices)
        samples = [train_seq[i] for i in indices]
        labels = [train_label[i] for i in indices]
        plt.figure(2)
        trial = 0
        for sample, label in zip(samples, labels):
            self._predict_shared.set_value([sample])
            p = self._predict_slope()
            plt.scatter(trial, p, c='b')
            plt.scatter(trial, label, c='r')
            plt.pause(0.05)
            trial += 1
        plt.waitforbuttonpress()


if __name__ == "__main__":

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
    # shuffle(z_data)
    data, labels = zip(*z_data)
    train_data = data[:int(0.7 * len(data))]
    train_labels = labels[:int(0.7 * len(labels))]
    valid_data = data[int(0.7 * len(data)) + 1: int(0.9 * len(data))]
    valid_labels = labels[int(0.7 * len(labels)) + 1: int(0.9 * len(labels))]
    test_data = data[int(0.9 * len(data)) + 1:]
    test_labels = labels[int(0.9 * len(labels)) + 1:]

    use_lstm = False
    use_DNN = True

    # LSTM
    if use_lstm:
        lstm_predictor = PredictorLSTM([100])
        if not lstm_predictor.load_model():
            lstm_predictor.train(train_data, train_labels)
            lstm_predictor.save_model()
            lstm_predictor.validate_model(valid_data=valid_data, valid_labels=valid_labels)
    if use_DNN:
        DNN_predictor = PredictorDNN([32])
        DNN_predictor.train(train_data, train_labels)
        #DNN_predictor.save_model()
        # if not DNN_predictor.load_model():
        #     DNN_predictor.train(train_data, train_labels)
        #     DNN_predictor.save_model()

