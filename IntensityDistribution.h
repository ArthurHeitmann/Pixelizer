#pragma once
#include <vector>
#include <array>

class IntensityDistribution
{
private:
	/*
	 * paremters for normal distribution function
	 * 0: hue function
	 * 1: saturation and value function
	 */
	int valueSpan[2];
	/*
	 * list/vector of all hsv values
	 */
	std::vector<std::array<float, 3>> values;
	/*
	 * stores all x and y values of the normal function
	 * for all 3 hsv channels
	 */
	std::array<std::vector<float>, 3> distribution;

public:
	/*
	 * hsvValues: hsv values of all pixels of the image that is being analyzed
	 * valueSpan[2]: paremters for normal distribution function
	 */
	IntensityDistribution(std::vector<std::array<float, 3>> hsvValues, int valueSpan[2]);
	/*
	 * calculates all values in the given intervall for the normal distribution function of the given channel.
	 * channel: 0: hue; 1: saturation; 2: value/brightness
	 * valueRange: the intervall of the values that are calculated (hue: 360, sat/val: 100)
	 * restartRange: to simulate color circle. Add the value from opposite side of the graph, if within given range
	 * restartAt: the x coordinate at which it will be restarted (if 0, then no restarting)
	 */
	void calcAllValues(int channel, int valueRange, int restartRange = 0, int restartAt = 0);
	/*
	 * calculate the value of the normal function, at x, at channel x
	 */
	float calcValueAt(int channel, int x);
	/*
	 * searches for all maximas on the graph of the channel.
	 */
	std::vector<std::array<float, 2>> findMaxima(int channel);
	/*
	 * average values of a channel
	 */
	float average(int channel);
	/*
	 * average deviation of the values of a channel
	 */
	float deviation(int channel, float compareValue);
	/*
	 * prints out the function. Can be used in a graphing application to visualize the color distribution.
	 */
	void printFunction(int channel, bool mirror = false);
};

