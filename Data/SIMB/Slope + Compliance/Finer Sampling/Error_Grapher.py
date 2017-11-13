import matplotlib.pyplot as plt
import cPickle as pickle
import numpy as np
import matplotlib.ticker as ticker


import sys
maj = sys.version_info

import math

version = 2

if maj[0] >= 3:
	import _pickle as pickle
	import importlib.machinery
	import types
	version = 3
else:
	import cPickle as pickle
	import imp


if version == 3:
	loader = importlib.machinery.SourceFileLoader('DatabaseInterface', '../../../DatabaseInterface.py')
	DBI = types.ModuleType(loader.name)
	loader.exec_module(DBI)
else:
	DBI = imp.load_source('DatabaseInterface', '../../../DatabaseInterface.py')


def plot_validation_errors(title, v_losses, axis=[0, 20, 0, 0.25]):
	plt.figure(1)

	plt.title(title)

	plt.xlabel('Epoch')
	plt.ylabel('Validation Loss')

	c = ['b', 'g', 'r', 'c', 'm', 'y']

	color = c[0]

	min_loss = np.argmin(v_losses[0])
	xy_pt = (min_loss, v_losses[0][min_loss])
	xy_text = (min_loss-1, v_losses[0][min_loss]+0.02)
	arrowprops = dict(
		facecolor=color,
		shrink=0.01
	)

	plt.annotate(
		'min: ' + str(v_losses[0][min_loss]),
		xy=xy_pt,
		xytext=xy_text,
		arrowprops=arrowprops
	)
	plt.plot(v_losses[0], label='trial 1', c=color)

	color = c[1]

	min_loss = np.argmin(v_losses[1])
	xy_pt = (min_loss, v_losses[1][min_loss])
	xy_text = (min_loss - 1, v_losses[1][min_loss] + 0.02)
	arrowprops = dict(
		facecolor=color,
		shrink=0.01
	)

	plt.annotate(
		'min: ' + str(v_losses[1][min_loss]),
		xy=xy_pt,
		xytext=xy_text,
		arrowprops=arrowprops
	)
	plt.plot(v_losses[1], label='trial 2', c=color)

	color = c[2]

	min_loss = np.argmin(v_losses[2])
	xy_pt = (min_loss, v_losses[2][min_loss])
	xy_text = (min_loss, v_losses[2][min_loss] - 0.02)
	arrowprops = dict(
		facecolor=color,
		shrink=0.01
	)

	plt.annotate(
		'min: ' + str(v_losses[2][min_loss]),
		xy=xy_pt,
		xytext=xy_text,
		arrowprops=arrowprops
	)
	plt.plot(v_losses[2], label='trial 3', c=color)

	plt.legend()
	plt.axis(axis)
	plt.axes().yaxis.set_major_locator(ticker.MultipleLocator(0.05))
	#plt.axes().yaxis.set_minor_locator(ticker.MultipleLocator(0.01))
	plt.axes().xaxis.set_major_locator(ticker.MultipleLocator(5))
	plt.axes().xaxis.set_minor_locator(ticker.MultipleLocator(1))

	plt.grid(True)
	plt.show()


def plot_single_test_predictions(model_file, input_data, labels, title, starting_idx=3000, end_idx=5000, axis=[0, 2000, -2, 2]):

	from keras.models import load_model

	model = load_model(model_file)

	predictions = model.predict_on_batch(
		x=input_data[starting_idx : end_idx]
	)

	plt.figure(1)
	plt.title(title)
	plt.xlabel('Input window')
	plt.ylabel('Normalized Output')

	plt.axis(axis)

	gt = labels[starting_idx : end_idx]

	for prediction in range(0, len(predictions)):
		p_vec = predictions[prediction]
		a_vec = gt[prediction]

		plt.subplot(111)
		plt.scatter(prediction, p_vec[-1], c='b', alpha=0.6)
		plt.scatter(prediction, a_vec[-1], c='r', alpha=0.6)

		plt.plot([prediction, prediction], [p_vec[-1], a_vec[-1]], color='k', alpha=0.3)

	plt.show()


def plot_multiple_test_predictions(model_file, input_data, slope_labels, compliance_labels, starting_idx=3000, end_idx=5000):
	from keras.models import load_model

	model = load_model(model_file)

	predictions = model.predict_on_batch(
		x=input_data[starting_idx: end_idx]
	)

	plt.figure(1)
	plt.subplot(211)
	plt.title('Slope Test Output')
	plt.xlabel('Input window')
	plt.ylabel('Normalized Output')
	plt.axis([0, 2000, -2.5, 2.5])

	plt.subplot(212)
	plt.title('Compliance Test Output')
	plt.xlabel('Input window')
	plt.ylabel('Normalized Output')
	plt.axis([0, 2000, -2, 2])

	slope_gt = slope_labels[starting_idx: end_idx]
	compliance_gt = compliance_labels[starting_idx : end_idx]

	slope_predictions = predictions[0]
	ground_predictions = predictions[1]

	print('# of slope predictions: ' + str(len(slope_predictions)))

	for prediction in range(0, len(slope_predictions)):
		s_p_vec = slope_predictions[prediction]
		c_p_vec = ground_predictions[prediction]
		s_vec = slope_gt[prediction]
		c_vec = compliance_gt[prediction]

		plt.subplot(211)
		plt.scatter(prediction, s_p_vec[-1], c='b', alpha=0.6)
		plt.scatter(prediction, s_vec[-1], c='r', alpha=0.6)
		plt.plot([prediction, prediction], [s_p_vec[-1], s_vec[-1]], color='k', alpha=0.3)

		plt.subplot(212)
		plt.scatter(prediction, c_p_vec[-1], c='b', alpha=0.6)
		plt.scatter(prediction, c_vec[-1], c='r', alpha=0.6)
		plt.plot([prediction, prediction], [c_p_vec[-1], c_vec[-1]], color='k', alpha=0.3)

	plt.show()


def plot_single_test_histogram_errors(model_file, input_data, labels, title, pickle_file='test_errors.p', axis=[-1.0, 1.0, 0, 1000]):

	try:
		errors = pickle.load(open(pickle_file, 'rb'))
	except Exception as e:

		from keras.models import load_model

		model = load_model(model_file)

		predictions = model.predict_on_batch(
			x=input_data
		)

		errors = np.subtract(predictions, labels)
		pickle.dump(errors, open(pickle_file, 'wb'))

	p_errors = errors[:,-1]
	p_errors = np.reshape(p_errors, (p_errors.shape[0], ))
	plt.figure(1)
	plt.title(title)

	n, bins, patches = plt.hist(p_errors, 500, facecolor='blue', alpha=0.6)

	plt.xlabel('Errors')
	plt.ylabel('Frequency')
	plt.axis(axis)
	plt.grid(True)

	plt.show()

	print('(m,v) = ({0}, {1})'.format(np.mean(p_errors), np.std(p_errors)))



def plot_multiple_test_histogram_errors(model_file, input_data, compliance_labels, slope_labels, pickle_file):
	try:
		slope_errors, compliance_errors = pickle.load(open(pickle_file, 'rb'))
	except Exception as e:

		from keras.models import load_model

		model = load_model(model_file)

		predictions = model.predict_on_batch(
			x=input_data
		)

		slope_predictions = predictions[0]
		compliance_predictions = predictions[1]

		slope_errors = np.subtract(slope_predictions, slope_labels)
		compliance_errors = np.subtract(compliance_predictions, compliance_labels)
		pickle.dump((slope_errors, compliance_errors), open(pickle_file, 'wb'))

	plt.figure(1)

	plt.subplot(211)
	plt.title('Slope Absolute Test Histogram')

	slope_p_errors = slope_errors[:,-1]
	slope_p_errors = np.reshape(slope_p_errors, (slope_p_errors.shape[0], ))

	n, bins, patches = plt.hist(slope_p_errors, 500, facecolor='blue', alpha=0.6)
	plt.xlabel('Errors')
	plt.ylabel('Frequency')
	plt.axis([-1.0, 1.0, 0, 2000])
	plt.grid(True)

	plt.subplot(212)
	plt.title('Compliance Absolute Test Histogram')
	compliance_p_errors = compliance_errors[:, -1]
	compliance_p_errors = np.reshape(compliance_p_errors, (compliance_p_errors.shape[0],))

	n, bins, patches = plt.hist(compliance_p_errors, 500, facecolor='blue', alpha=0.6)
	plt.xlabel('Errors')
	plt.ylabel('Frequency')
	plt.axis([-1.0, 1.0, 0, 1000])
	plt.grid(True)

	plt.show()

	print(
	'Slope (m,v) = ({0}, {1}), Compliance (m,v) = ({2}, {3})'.format(np.mean(slope_p_errors), np.std(slope_p_errors),
																	 np.mean(compliance_p_errors),
																	 np.std(compliance_p_errors)))


def generate_compliance_plots():

	model_file = 'compliance/trial 1/model-34-0.03141.hdf5'

	# Validation graphs
	# val_losses_trial_1 = pickle.load(open('compliance/trial 1/val_losses.p', 'rb'))
	# val_losses_trial_2 = pickle.load(open('compliance/trial 2/val_losses.p', 'rb'))
	# val_losses_trial_3 = pickle.load(open('compliance/trial 3/val_losses.p', 'rb'))
	# plot_validation_errors(
	# 	'Compliance Validation Losses vs Epoch',
	# 	[val_losses_trial_1, val_losses_trial_2, val_losses_trial_3],
	# 	axis=[0, 40, 0, 0.1]
	# )


	# Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	plot_single_test_predictions(model_file, input_data=test_data, labels=compliance_labels, title='Compliance Test Output')

	#Error Histogram
	plot_single_test_histogram_errors(model_file, input_data=test_data, labels=compliance_labels, title='Compliance Test Error Histogram', pickle_file='compliance/test_errors.p')


def generate_slope_plots():
	# Validation graphs
	val_losses_trial_1 = pickle.load(open('slope/trial 1/val_losses.p', 'rb'))
	val_losses_trial_2 = pickle.load(open('slope/trial 2/val_losses.p', 'rb'))
	val_losses_trial_3 = pickle.load(open('slope/trial 3/val_losses.p', 'rb'))
	plot_validation_errors('Slope Validation Losses vs Epoch',
						   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3],
						   axis=[0, 40, 0, 0.1]
						   )

	model_file = 'slope/trial 1/model-31-0.01687.hdf5'

	#Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	plot_single_test_predictions(
		model_file,
		input_data=test_data,
		labels=slope_labels,
		title='Slope Test Output',
		axis=[0, 2000, -2.7, 2.7]
	)

	#Error Histogram
	plot_single_test_histogram_errors(
		model_file,
		input_data=test_data,
		labels=slope_labels,
		title='Slope Test Error Histogram', pickle_file='slope/test_errors.p',
		axis=[-1.0, 1.0, 0, 2500]
	)


def generate_slope_compliance_plots():
	# Validation graphs
	# val_losses_trial_1 = pickle.load(open('slope_compliance/trial 1/val_losses.p', 'rb'))
	# val_losses_trial_2 = pickle.load(open('slope_compliance/trial 2/val_losses.p', 'rb'))
	# val_losses_trial_3 = pickle.load(open('slope_compliance/trial 3/val_losses.p', 'rb'))
	# plot_validation_errors('Slope + Compliance Validation Losses vs Epoch',
	# 					   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3])
	#
	model_file = 'slope_compliance/trial 1/model-35-0.04533.hdf5'

	# Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	# plot_multiple_test_predictions(
	# 	model_file=model_file,
	# 	input_data=test_data,
	# 	slope_labels=slope_labels,
	# 	compliance_labels=compliance_labels,
	# 	starting_idx=0,
	# 	end_idx=160
	# )

	#Error Histogram
	plot_multiple_test_histogram_errors(
		model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
		pickle_file='slope_compliance/test_errors.p'
	)


def generate_slope_compliance_hp_plots():
	# Validation graphs
	val_losses_trial_1 = pickle.load(open('slope_compliance_hp/trial 1/val_losses.p', 'rb'))
	val_losses_trial_2 = pickle.load(open('slope_compliance_hp/trial 2/val_losses.p', 'rb'))
	val_losses_trial_3 = pickle.load(open('slope_compliance_hp/trial 3/val_losses.p', 'rb'))
	plot_validation_errors('Slope + Compliance Validation Losses vs Epoch',
						   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3])

	model_file = 'slope_compliance_hp/trial 2/model-38-0.05293.hdf5'

	# Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	plot_multiple_test_predictions(
		model_file=model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
	)

	#Error Histogram
	plot_multiple_test_histogram_errors(
		model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
		pickle_file='slope_compliance_hp/test_errors.p'
	)


def generate_varying_window_plots(window_size):

	# Validation graphs
	# val_losses_trial_1 = pickle.load(open('varying_window/' + str(window_size) + '/trial 1/val_losses.p', 'rb'))
	# val_losses_trial_2 = pickle.load(open('varying_window/' + str(window_size) + '/trial 2/val_losses.p', 'rb'))
	# val_losses_trial_3 = pickle.load(open('varying_window/' + str(window_size) + '/trial 3/val_losses.p', 'rb'))
	# plot_validation_errors('Window Size = ' + str(window_size) + ' Validation Losses vs Epoch',
	# 					   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3],
	# 					   axis=[0, 20, 0, 0.80])

	model_file = 'varying_window/' + str(window_size) + '/trial 2/model-16-0.16923.hdf5'

	# Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	# plot_multiple_test_predictions(
	# 	model_file=model_file,
	# 	input_data=test_data,
	# 	slope_labels=slope_labels,
	# 	compliance_labels=compliance_labels,
	# )

	#Error Histogram
	plot_multiple_test_histogram_errors(
		model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
		pickle_file='varying_window/' + str(window_size) + '/test_errors.p'
	)


def generate_no_forces_plots():
	# Validation graphs
	# val_losses_trial_1 = pickle.load(open('No Forces/trial 1/val_losses.p', 'rb'))
	# val_losses_trial_2 = pickle.load(open('No Forces/trial 2/val_losses.p', 'rb'))
	# val_losses_trial_3 = pickle.load(open('No Forces/trial 3/val_losses.p', 'rb'))
	# plot_validation_errors('Slope + Compliance Validation Losses vs Epoch',
	# 					   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3])

	model_file = 'No Forces/trial 1/model-19-0.11146.hdf5'

	# Test Prediction

	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)

	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	plot_multiple_test_predictions(
		model_file=model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
	)

	# Error Histogram
	plot_multiple_test_histogram_errors(
		model_file,
		input_data=test_data,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
		pickle_file='No Forces/test_errors.p'
	)


def generate_no_states_plots():
	# Validation graphs
	# val_losses_trial_1 = pickle.load(open('No State/trial 1/val_losses.p', 'rb'))
	# val_losses_trial_2 = pickle.load(open('No State/trial 2/val_losses.p', 'rb'))
	# val_losses_trial_3 = pickle.load(open('No State/trial 3/val_losses.p', 'rb'))
	# plot_validation_errors('Slope + Compliance Validation Losses vs Epoch',
	# 					   [val_losses_trial_1, val_losses_trial_2, val_losses_trial_3])
	#
	model_file = 'No State/trial 1/model-19-0.09984.hdf5'
	#
	# # Test Prediction
	#
	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)
	#
	slope_labels, compliance_labels = DBI.split_labels(test_labels)
	#
	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)
	#
	forces = np.concatenate((lf_forces, rf_forces), 2)
	#
	# plot_multiple_test_predictions(
	# 	model_file=model_file,
	# 	input_data=forces,
	# 	slope_labels=slope_labels,
	# 	compliance_labels=compliance_labels,
	# )

	# Error Histogram
	plot_multiple_test_histogram_errors(
		model_file,
		input_data=forces,
		slope_labels=slope_labels,
		compliance_labels=compliance_labels,
		pickle_file='No State/test_errors.p'
	)


def errors_from_mean_values_for_validation():
	valid_data, valid_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_val.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=True
	)

	means = pickle.load(open('means.p', 'rb'))
	variances = pickle.load(open('variances.p', 'rb'))

	mean_slope = means['GROUND_SLOPE']
	mean_compliance = means['GROUND_STIFFNESS']

	var_slope = variances['GROUND_SLOPE']
	var_compliance = variances['GROUND_STIFFNESS']

	slope_labels, compliance_labels = DBI.split_labels(valid_labels)

	n_slopes = normalize_data(slope_labels, mean_slope, var_slope)
	n_compliances = normalize_data(compliance_labels, mean_compliance, var_compliance)

	avg_error = 0
	avg_slope_error = 0
	avg_compliance_error = 0

	for i, (slope_window, compliance_window) in enumerate(zip(n_slopes, n_compliances)):
		slope_errors = np.average(np.abs(slope_window))
		compliance_errors = np.average(np.abs(compliance_window))
		total_error = slope_errors + compliance_errors
		count = i+1

		avg_slope_error = avg_slope_error * (count - 1) / count + slope_errors / count
		avg_compliance_error = avg_compliance_error * (count - 1) / count + compliance_errors / count
		avg_error = avg_error * (count - 1)/count + total_error/count

	print(avg_slope_error, avg_compliance_error, avg_error)


def normalize_data(data, mean, variance):
	return [[standardize_data(dat[0], mean, variance) for dat in window] for window in data ]


def standardize_data(dat, mean, variance):
	if variance == 0:
		return 0
	return (dat - mean) / math.sqrt(variance)


def plot_inputs(index, num_features):
	test_data, test_labels, foot_forces = DBI.prepare_data(
		db_str='../../../samples_33_test.db',
		num_seq=None,
		mode='normalize',
		include_forces=True,
		dump=False
	)
	slope_labels, compliance_labels = DBI.split_labels(test_labels)

	lf_forces = foot_forces[0]
	rf_forces = foot_forces[1]

	lf_forces = DBI.avg_down_foot_forces(lf_forces)
	rf_forces = DBI.avg_down_foot_forces(rf_forces)

	forces = np.concatenate((lf_forces, rf_forces), 2)

	test_data = np.concatenate((test_data, forces), 2)

	start_idx = 230

	plt_data = test_data[start_idx:start_idx+index]
	plt_slope_labels = [l[-1] for l in slope_labels[start_idx:start_idx+index]]
	plt_comp_labels = [l[-1] for l in compliance_labels[start_idx:start_idx+index]]

	features = plt_data[:, :,:num_features]

	to_plt = np.zeros((30 + index-1, num_features))
	plt_slope = np.reshape(plt_slope_labels, (len(plt_slope_labels),))
	plt_comp = np.reshape(plt_comp_labels, (len(plt_comp_labels),))

	to_plt[:30, :] = features[0, :, :]
	to_plt[30:, :] = features[1:, -1, :]

	plt.figure(1)
	plt.subplot(211)
	plt.title('Features')
	colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']
	markers = ['1', '2', '*', 's', 'p', 'D', 'x']
	plots = []
	for f in range(0, num_features):
		plots.append(plt.scatter(np.arange(0, to_plt.shape[0]), to_plt[:,f], c=colors[f % 7], marker=markers[f%7], label=str(f)))
	plt.legend(plots, ['Feature {0}'.format(a) for a in range(0, num_features)])
	plt.xlim([0, 29 + index])
	plt.ylim([-4, 4])
	plt.xlabel('Time')
	plt.ylabel('Normalized Value')

	plt.subplot(212)
	plt.title('Labels')
	p1 = plt.scatter(np.arange(29, 29+index), plt_slope, c='r', alpha=0.6, label='slope')
	p2 = plt.scatter(np.arange(29, 29+index), plt_comp, c='b', alpha=0.6, label='compliance')
	plt.xlim([0, 29+index])
	plt.ylim([-4, 4])
	plt.legend([p1, p2], ['slope', 'compliance'])
	plt.xlabel('Time')
	plt.ylabel('Normalized Value')
	plt.show()


if __name__ == '__main__':
	# compliance
	#generate_compliance_plots()
	# slope
	#generate_slope_plots()
	# slope + compliance
	#generate_slope_compliance_plots()
	# slope + compliance hp
	#generate_slope_compliance_hp_plots()
	# varying window
	generate_varying_window_plots(3)
	#errors_from_mean_values_for_validation()
	#generate_no_forces_plots() # Only State
	#generate_no_states_plots() # Only foot pressure forces
	#plot_inputs(100, 3)


