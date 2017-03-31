import sys
maj = sys.version_info

import numpy as np

version = 2

if maj[0] >= 3:
	import _pickle as pickle
	import importlib.machinery
	import types
	version = 3
else:
	import cPickle as pickle
	import imp

import matplotlib.pyplot as plt


def graph(history_vfiles):
	val_losses = []
	colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k', 'w']
	plots = []
	labels = []
	minimums = []
	for i, h_file in enumerate(history_vfiles):
		label = 'Trial ' + str(i + 1)
		data = pickle.load(open(h_file, 'rb'))
		plot, = plt.plot(data, colors[i % len(colors)], label=label)
		plots.append(plot)
		labels.append(label)
		minimums.append(data[np.argmin(data)])
		plt.text()


	plt.plot(val_losses)
	plt.ylabel('Validation Loss')
	plt.xlabel('Trial')
	plt.title('Validation Loss vs. Trial')
	plt.legend(plots, labels)
	plt.show()

if __name__ == '__main__':
	graph(['slope_compliance/trial 1/val_losses.p',
		   'slope_compliance/trial 2/val_losses.p',
		   'slope_compliance/trial 3/val_losses.p' ] )