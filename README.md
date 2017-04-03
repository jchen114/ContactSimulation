# Leveraging LSTM networks for Terrain Properties Prediction
This is a physics based simulation developed in C++ using Bullet Physics and OpenGL. The bipedal walker is controlled using the strategy proposed by SIMBICON in this [paper](http://www.cs.ubc.ca/~van/papers/Simbicon.htm). Online demo of strategy [here](https://jchen114.github.io/SIMBICON-Web/)! Developed by me. 

In the simulation, the bipedal walker tries to predict the slope and compliance that it is walking on. 
To do this, I developed my own contact model to model the compliance of the terrain using springs.
I then build data where each state input contains information about the Biped agent such as rigid body orientation and angular velocity as well as the forces that it feels on its feets by my compliance model. 

Using a history of these states, I can then train a multilayer LSTM network offline to predict the slope and compliance of the terrain! Predictions can then be made in real time by the bipedal walker.

