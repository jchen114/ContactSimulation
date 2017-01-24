import lasagne
import theano.tensor as T
import numpy as np
import cPickle as pickle
import theano
import matplotlib.pyplot as plt
import math as math

import DatabaseInterface as DI

from random import shuffle


def slope_generator(step_size, min_steps, max_steps):
    while True:
        num_steps = np.random.randint(min_steps, max_steps)
        xs = step_size * np.arange(1, num_steps)
        #start = np.random.uniform(low=-1.0, high=1.0)
        start = np.random.randint(low=-5, high=5)
        slope = np.random.choice([-2, -1, 0, 1, 2], p=[0.2, 0.2, 0.2, 0.2, 0.2])
        y = [start + slope*x for x in xs]
        yield y, slope

# Experiment to see if predicting at the end of a sequence is better than
# predicting for each element in a sequence given the input
if __name__ == "__main__":

    lasagne.random.set_rng(np.random.RandomState(1))

    sequences = T.dtensor3('sequences')
    #targets = T.dvector('slope_targets')
    targets = T.ivector('slope_targets')

    print "-- Building Network --"

    example = np.random.ranf(size=(1, 15, 1)).astype(dtype=np.float64)

    l_in = lasagne.layers.InputLayer(shape=(None, None, 1), input_var=sequences)

    print "after input: "
    print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

    hidden_units = [100, 100]

    i = 0

    ret_final = False
    # LSTM layers
    for units in hidden_units:
        if i == len(hidden_units) - 1:
            ret_final = False
        # Slope
        l_in = lasagne.layers.LSTMLayer(
            incoming=l_in,
            num_units=units,
            nonlinearity=lasagne.nonlinearities.tanh,
            only_return_final=ret_final)
        l_in = lasagne.layers.dropout(
            incoming=l_in,
            p=0.1
        )
        i += 1
        print "After LSTM: "
        print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

    if not ret_final:
        l_in = lasagne.layers.ReshapeLayer(l_in, (-1, hidden_units[-1]))
        print "After reshape: "
        print lasagne.layers.get_output(l_in).eval({sequences: example}).shape

    # Output
    # l_out = lasagne.layers.DenseLayer(
    #     incoming=l_in,
    #     num_units=1,
    #     nonlinearity=lasagne.nonlinearities.linear
    # )
    # Soft max classifier
    l_out = lasagne.layers.DenseLayer(
        incoming=l_in,
        num_units=5,
        nonlinearity=lasagne.nonlinearities.softmax
    )

    print "After Dense: "
    print lasagne.layers.get_output(l_out).eval({sequences:example}).shape

    output = lasagne.layers.get_output(l_out)
    #loss = T.mean(0.5 * lasagne.objectives.squared_error(targets, output))
    loss = lasagne.objectives.categorical_crossentropy(output, targets).mean()

    params = lasagne.layers.get_all_params(l_out, trainable=True)

    updates = lasagne.updates.rmsprop(loss, params, learning_rate=0.1)

    seq_shared = theano.shared(
        np.zeros((1, 50, 1), dtype=np.float64)
    )

    targets_shared = theano.shared(
        np.zeros(1, dtype=np.int32)
    )

    # Train functions
    train = theano.function(
        inputs=[],
        outputs=[loss, output],
        updates=updates,
        givens={
            sequences: seq_shared,
            targets: targets_shared
        },
        allow_input_downcast=True
    )

    # Predict
    predict = theano.function(
        inputs=[],
        outputs=output,
        givens={
            sequences: seq_shared
        }
    )

    slope_gen = slope_generator(0.5, 10, 20)

    plt.ion()
    idx = 0
    plt.figure(1)
    plt.subplot(211)
    plt.title('Loss')
    plt.subplot(212)
    plt.title('Predictions')
    for epoch in range (0, 10):
        for i in range (0, 100):
            sequence, slope = next(slope_gen)
            sequence = [[el] for el in sequence]
            label = slope + 2
            seq_shared.set_value([sequence])
            if not ret_final:
                targets_shared.set_value([label] * len(sequence))
            else:
                targets_shared.set_value([label])
            loss, ps = train()
            plt.subplot(211)
            plt.scatter(idx, loss)
            plt.subplot(212)
            p = np.argmax(ps[-1])
            plt.scatter(idx, p, c='b')
            plt.scatter(idx, label, c='r')
            plt.pause(0.05)
            idx += 1
    plt.waitforbuttonpress()

    np.savez('model_slope_lstm.npz', *lasagne.layers.get_all_param_values(l_out))

    # Validation
    with np.load('model_slope_lstm.npz') as f:
        param_values = [f['arr_%d' % i] for i in range(len(f.files))]
        lasagne.layers.set_all_param_values(l_out, param_values)

    plt.figure(2)
    plt.title('Validation')
    for i in range(0, 30):
        sequence, slope = next(slope_gen)
        sequence = [[el] for el in sequence]
        label = slope
        seq_shared.set_value([sequence])
        p = predict()
        plt.scatter(i, p, c='g')
        plt.scatter(i, label, c='r')
        plt.pause(0.05)
    plt.waitforbuttonpress()
