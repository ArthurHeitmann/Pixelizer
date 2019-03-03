#pragma once
#include <vector>
#include <array>

class IntensityDistribution
{
private:
	int valueSpan[2];
	std::vector<std::array<float, 3>> values;
	std::vector<std::vector<float>> distribution;

public:
	IntensityDistribution(std::vector<std::array<float, 3>> hsvValues, int valueSpan[2]);

	void calcAllValues(int channel, int valueRange, int restartRange = 0, int restartAt = 0);
	float calcValueAt(int channel, int x);
	std::vector<std::array<float, 2>> findMaxima(int channel);
	float average(int channel);
	float deviation(int channel, float compareValue);
	void printFunction(int channel, bool mirror = false);
};

