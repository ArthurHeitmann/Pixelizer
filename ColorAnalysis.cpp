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
	intDest.calcAllValues(1, 100);
	intDest.calcAllValues(2, 100);
	//intDest.printFunction(0, true);
	//std::cout << "Maxima for Hue:\n";
	maxima = intDest.findMaxima(0);
	sortMaxima();
	/*std::cout << "Maxima for Saturation:\n";
	intDest.findMaxima(1);
	std::cout << "Maxima for Value:\n";
	intDest.findMaxima(2);*/
	avrSat = intDest.average(1);
	//float avrSatDevia = intDest.deviation(1, avrSat);
	avrVal = intDest.average(2);
	//float avrValDevia = intDest.deviation(2, avrVal);
	/*std::cout << "Average Saturation:\n";
	std::cout << avrSat << std::endl;
	//std::cout << avrSatDevia << std::endl;
	std::cout << "Average Value:\n";
	std::cout << avrVal << std::endl;*/
	//std::cout << avrValDevia << std::endl;
	/*int test = 0;
	std::cout << "Hue func: ";
	std::cin >> test;
	intDest.printFunction(0);
	std::cout << "Sat func: ";
	std::cin >> test;
	intDest.printFunction(1);
	std::cout << "Val func: ";
	std::cin >> test;
	intDest.printFunction(2);*/

}


std::array<float, 3> ColorAnalysis::rgbToHsv(int* rgb)
{
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


	return out;
}

void ColorAnalysis::sortMaxima()
{
	std::map<float, float> tmpMaximaMap;
	std::vector<std::array<float, 2>> newMaxima;
	for (std::array<float, 2> maximaPoint : maxima) {
		tmpMaximaMap.insert(std::pair<float, float>(maximaPoint[1], maximaPoint[0]));
	}

	for (auto maximaPoint : tmpMaximaMap) {
		newMaxima.push_back({maximaPoint.second, maximaPoint.first});
	}

	maxima = newMaxima;
}