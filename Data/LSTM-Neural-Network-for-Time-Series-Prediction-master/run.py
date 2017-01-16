import lstm_lasagne
#import lstm
import time
import matplotlib.pyplot as plt
import numpy as np


def load_data(filename, seq_len, normalise_window):
    f = open(filename, 'rb').read()
    data = f.split('\n')
    data = [float(dat) for dat in data]
    sequence_length = seq_len + 1
    result = []
    for index in range(len(data) - sequence_length):
        result.append(data[index: index + sequence_length])

    if normalise_window:
        result = normalise_windows(result)

    result = np.array(result)

    row = round(0.9 * result.shape[0])
    train = result[:row, :]
    np.random.shuffle(train)
    x_train = train[:, :-1]
    y_train = train[:, -1]
    x_test = result[row:, :-1]
    y_test = result[row:, -1]

    x_train = np.reshape(x_train, (x_train.shape[0], x_train.shape[1], 1))
    x_test = np.reshape(x_test, (x_test.shape[0], x_test.shape[1], 1))

    return [x_train, y_train, x_test, y_test]


def normalise_windows(window_data):
    normalised_data = []
    for window in window_data:
        normalised_window = [((float(p) / float(window[0])) - 1) for p in window]
        normalised_data.append(normalised_window)
    return normalised_data

def plot_results(predicted_data, true_data):
    fig = plt.figure(facecolor='white')
    ax = fig.add_subplot(111)
    ax.plot(true_data, label='True Data')
    plt.plot(predicted_data, label='Prediction')
    plt.legend()
    plt.show()


def plot_results_multiple(predicted_data, true_data, prediction_len):
    # type: (object, object, object) -> object
    plt.ion()
    fig = plt.figure(facecolor='white')
    ax = fig.add_subplot(111)
    ax.plot(true_data, label='True Data')
    # Pad the list of predictions to shift it in the graph to it's correct start
    for i, data in enumerate(predicted_data):
        padding = [None for p in xrange(i * prediction_len)]
        plt.plot(padding + data, label='Prediction')
        plt.legend()
    plt.show()
    plt.waitforbuttonpress()


# Main Run Thread
if __name__ == '__main__':

    keras = False
    lasagne = True

    epochs = 10
    seq_len = 50

    print '> Loading data... '

    X_train, y_train, X_test, y_test = load_data('sinwave.csv', seq_len, False)

    if keras:
        global_start_time = time.time()

        print '> Data Loaded. Compiling...'

        # model = lstm.build_model([1, 50, 100, 1])
		#
        # model.fit(
        #     X_train,
        #     y_train,
        #     batch_size=512,
        #     nb_epoch=epochs,
        #     validation_split=0.05)
		#
        # predictions = lstm.predict_sequences_multiple(model, X_test, seq_len, 50)
        # predicted = lstm.predict_sequence_full(model, X_test, seq_len)
        # predicted = lstm.predict_point_by_point(model, X_test)

        print 'Training duration (s) : ', time.time() - global_start_time
        #plot_results_multiple(predictions, y_test, 50)

    if lasagne:
        model_lasagne = lstm_lasagne.LasagneLSTM([50, 100])
        model_lasagne.train(
            train_dat=X_train,
            train_labels=y_train,
            batch_size=512,
            epochs=epochs)
        predictions = model_lasagne.predict_sequences_multiple(X_test, seq_len, 50)
        plot_results_multiple(predictions, y_test, 50)
