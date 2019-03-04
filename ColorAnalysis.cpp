#include "pch.h"
#include "ColorAnalysis.h"
#include <vector>
#include "IntensityDistribution.h"
#include <iostream>
#include <map>

ColorAnalysis::ColorAnalysis(const char* imgPath, bool autoExecute, int downscalingResolution)
{
	downscaleResolution = downscalingResolution;
	img = CImg<unsigned char>(imgPath);
	if (img.width() != img.height()) {
		if (img.height() > img.width())
			img.crop(0, (int)(img.height() - img.width()) / 2, img.width(), (int)(img.width() + (img.height() - img.width()) / 2));
		else
			img.crop((int)(img.width() - img.height()) / 2, 0, img.height(), (int)(img.height() + (img.width() - img.height()) / 2));
	}
	img.resize(downscaleResolution, downscaleResolution, 1, 3, 5);
	//img.display();
	//img.RGBtoHSV();
	if (autoExecute) {
		analyseImg();

		//img.display();
	}
}

void ColorAnalysis::analyseImg()
{
	std::vector<std::array<float, 3>> hsvValues;
	for (int y = 0; y < downscaleResolution; y++)
	{
		for (int x = 0; x < downscaleResolution; x++)
		{
			int rgbValue[] = { img(x, y, 0, 0), img(x, y, 0, 1), img(x, y, 0, 2) };
			hsvValues.emplace_back(rgbToHsv(rgbValue));
		}
	}
	int valueSpan[] = {200, 90};

	IntensityDistribution intDest(hsvValues, valueSpan);
	intDest.calcAllValues(0, 360, 50, 360);
	maxima = intDest.findMaxima(0);
	sortMaxima();
	avrSat = intDest.average(1);
	avrVal = intDest.average(2);

}


std::array<float, 3> ColorAnalysis::rgbToHsv(int* rgb)
{
	//straight from so
	std::array<float, 3> hsv;
	int min = rgb[0] < rgb[1] ? rgb[0] : rgb[1];
	min = min < rgb[2] ? min : rgb[2];
	int max = rgb[0] > rgb[1] ? rgb[0] : rgb[1];
	max = max > rgb[2] ? max : rgb[2];

	hsv[2] = (float) max / 255;
	int delta = max - min;
	if (delta == 0)
	{
		hsv[0] = 0;
		hsv[1] = 0;
		return hsv;
	}

	if (max > 0)
	{
		hsv[1] = (float) delta / max;
	}
	else
	{
		hsv[0] = 0;
		hsv[1] = 0;
		return hsv;
	}

	if (rgb[0] == max)
		hsv[0] = (float)(rgb[1] - rgb[2]) / delta;
	else if (rgb[1] == max)
		hsv[0] = (float)2 + (rgb[0] - rgb[1]) / delta;
	else
		hsv[0] = 4 + (float) (rgb[0] - rgb[1]) / delta;

	hsv[0] *= 60;
	if (hsv[0] < 0)
		hsv[0] += 360;

	return hsv;
}

json ColorAnalysis::dataSummary()
{
	json out = json::object();

	out["hue_maximas"] = json::array();
	for (std::array<float, 2> maximaPoint : maxima) {
		out["hue_maximas"].push_back(maximaPoint);
	}

	out["avr_sat"] = avrSat;
	out["avr_val"] = avrVal;

	if (avrSat < 0.1 || avrVal < 0.07)												//extremely low sat or brightnes --> hue cannot be seen anymore
			out["category"] = 2;													//grayscale
	else if (maxima.size() == 1 || 
		maxima[0][1] / maxima[1][1] > 1.25 || 
		colorCircleDistance(maxima[0][0], maxima[1][0]) < 50)
		out["category"] = 0;													//strong main color in image
	else
		out["category"] = 1;													//multiple important colors --> this image doesn't have a clear main color
	



	return out;
}

int ColorAnalysis::colorCircleDistance(int hue1, int hue2)
{
	int minDistance = abs(hue1 - hue2);


	return minDistance > 180 ? 360 - minDistance : minDistance;
}

void ColorAnalysis::sortMaxima()
{
	std::map<float, float, std::greater<float>> tmpMaximaMap;
	std::vector<std::array<float, 2>> newMaxima;
	for (std::array<float, 2> maximaPoint : maxima) {
		tmpMaximaMap.insert(std::pair<float, float>(maximaPoint[1], maximaPoint[0]));
	}

	for (auto maximaPoint : tmpMaximaMap) {
		newMaxima.push_back({maximaPoint.second, maximaPoint.first});
	}

	maxima = newMaxima;
}