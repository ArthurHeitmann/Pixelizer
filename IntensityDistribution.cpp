#include "pch.h"
#include "IntensityDistribution.h"
#include <string>
#include <iostream>
#include <string>


IntensityDistribution::IntensityDistribution(std::vector<std::array<float, 3>> hsvValues, int valueSpan[2])
{
	this->valueSpan[0] = valueSpan[0];
	this->valueSpan[1] = valueSpan[1];
	values = hsvValues;
	
}

void IntensityDistribution::calcAllValues(int channel, int valueRange, int restartRange, int restartAt)
{
	for (int x = 0; x < valueRange; x++)
	{
		//calculate value at x and add it to the distribution list/vector
		distribution[channel].push_back(calcValueAt(channel, x));

		if (restartRange)
		{
			if (x < restartRange)
				distribution[channel][x] += calcValueAt(channel, restartAt + x);
			else if (x > restartAt - restartRange)
				distribution[channel][x] += calcValueAt(channel, x - restartAt);
		}
	}
}

float IntensityDistribution::calcValueAt(int channel, int x)
{
	float outValue = 0;
	if (channel == 0)
	{
		for (std::array<float, 3> hsvValue : values)
		{
			//normal distribution function * saturation and brightness at of that pixel
			//this makes "weak" pixels have less of an impact on the graph
			outValue += exp(pow(x - hsvValue[0], 2) / valueSpan[0] * (-1))
				* hsvValue[1]
				* hsvValue[2];
		}
	}
	else
	{
		for (std::array<float, 3> hsvValue : values)
		{
			outValue += exp(pow(x - hsvValue[channel] * 100, 2) / valueSpan[1] * (-1));
		}
	}

	return outValue;
}

std::vector<std::array<float, 2>> IntensityDistribution::findMaxima(int channel)
{
	std::vector<std::array<float, 2>> maxima;
	std::array<float, 2> previousValue = {-1, -1};
	bool upwardDirection = true;

	//"walks" through all the graph
	for (int valueIndex = 0; valueIndex < distribution[channel].size(); valueIndex++)
	{
		//if the current value is lower than the previous and we were previously going upwards, the previous value is a maxima
		// > 10 to keep out unnoticeable colors
		if (distribution[channel][valueIndex] < previousValue[1] && upwardDirection && previousValue[1] > 10)
		{
			std::array<float, 2> tmpMaxima = {valueIndex - 1, distribution[channel][valueIndex - 1]};
			maxima.push_back(tmpMaxima);
			upwardDirection = false;
		}
		else if (distribution[channel][valueIndex] > previousValue[1])
		{
			upwardDirection = true;
		}

		previousValue[0] = valueIndex;
		previousValue[1] = distribution[channel][valueIndex];
	}

	return maxima;
}

float IntensityDistribution::average(int channel)
{
	float tmpSum = 0;
	for (std::array<float, 3> hsvValue : values) {
		tmpSum += hsvValue[channel];
	}

	return (float) tmpSum / values.size();
}

float IntensityDistribution::deviation(int channel, float compareValue)
{
	float tmpSum = 0;
	for (std::array<float, 3> hsvValue : values) {
		tmpSum += abs(hsvValue[channel] - compareValue);
	}

	return (float) (tmpSum / values.size());
}

void IntensityDistribution::printFunction(int channel, bool mirror)
{
	std::string str = "";
	if (channel == 0)
	{
		for (std::array<float, 3> hsvValue : values)
		{
			str += "+e^(-(x-";
			str += std::to_string(hsvValue[0]);
			str += ")^2/200)*";
			str += std::to_string(hsvValue[1] * hsvValue[2]);
		}
		if (mirror)
		{
			for (std::array<float, 3> hsvValue : values)
			{
				str += "+(e^(-(x-360-";
				str += std::to_string(hsvValue[0]);
				str += ")^2/200)*";
				str += "+e^(-(x+360-";
				str += std::to_string(hsvValue[0]);
				str += ")^2/200))*";
				str += std::to_string(hsvValue[1] * hsvValue[2]);
			}
		}
	}
	else
	{
		for (std::array<float, 3> hsvValue : values)
		{
			str += "+e^(-(x-";
			str += std::to_string(hsvValue[channel] * 100);
			str += ")^2/90)";
		}
	}

	std::cout << str << std::endl;
}
