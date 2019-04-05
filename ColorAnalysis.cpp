#include "pch.h"
#include "ColorAnalysis.h"
#include <vector>
#include "IntensityDistribution.h"
#include <iostream>
#include <map>
#include "PixTools.h"

ColorAnalysis::ColorAnalysis(const char* imgPath, bool autoExecute, int downscalingResolution)
{
	downscaleResolution = downscalingResolution;
	img = CImg<unsigned char>(imgPath);
	//crop to 1:1 ratio
	if (img.width() != img.height()) {
		if (img.height() > img.width())
			img.crop(0, (img.height() - img.width()) / 2, img.width(), (img.width() + (img.height() - img.width()) / 2));
		else
			img.crop((img.width() - img.height()) / 2, 0, (img.height() + (img.width() - img.height()) / 2), img.height());
	}
	cropOriginalImg = CImg<unsigned char>(img);
	cropOriginalImg.crop(0, 0, cropOriginalImg.width() * 2, cropOriginalImg.height());
	img.resize(downscaleResolution, downscaleResolution, 1, 3, 5);
	if (autoExecute) {
		analyseImg();
	}
}

void ColorAnalysis::analyseImg()
{
	//convert all rgb values to hsv values
	std::vector<std::array<float, 3>> hsvValues;
	for (int y = 0; y < downscaleResolution; y++)
	{
		for (int x = 0; x < downscaleResolution; x++)
		{
			int rgbValue[] = { img(x, y, 0, 0), img(x, y, 0, 1), img(x, y, 0, 2) };
			hsvValues.emplace_back(PixTools::rgbToHsv(rgbValue));
		}
	}
	int valueSpan[] = {200, 90};

	IntensityDistribution intDest(hsvValues, valueSpan);
	intDest.calcAllValues(0, 360, 50, 360);
	hueMaxima = intDest.findMaxima(0);
	sortMaxima();
	avrSat = intDest.average(1);
	avrVal = intDest.average(2);
	avrValScore =  intDest.calcValueAt(2, avrVal);
}



json ColorAnalysis::dataSummary()
{
	json out = json::object();

	//3 different categories:
	//	- 0: there is one main color in the image
	//	- 1: there are too many different colors --> image shouldn't be used
	//	- 2: Image is (close to) grayscale or so dark, that it doesn't matter anyways
	
	if (hueMaxima.size() != 0) {
		out["max_hue"] = hueMaxima[0][0];
		out["max_hue_score"] = hueMaxima[0][1];
	}

	out["avr_val_score"] = avrValScore;

	out["avr_sat"] = avrSat;
	out["avr_val"] = avrVal;

	
	if (avrSat < 0.025 || avrVal < 0.03 || hueMaxima.size() == 0)
		out["category"] = 2;
	else if (hueMaxima.size() == 1)
		out["category"] = 0;
	else if (hueMaxima[0][1] > 20 && hueMaxima[0][1] / hueMaxima[1][1] < 3 && PixTools::distanceOnCircle((int)hueMaxima[0][0], (int)hueMaxima[1][0]) > 40)
		out["category"] = 1;
	else if (hueMaxima[0][1] <= 20 && hueMaxima[0][1] / hueMaxima[1][1] < 2 && PixTools::distanceOnCircle((int)hueMaxima[0][0], (int)hueMaxima[1][0]) > 40)
		out["category"] = 1;
	else if (hueMaxima.size() > 2 && hueMaxima[2][1] > 4 && hueMaxima[0][1] / hueMaxima[2][1] < 3)
		out["category"] = 1;
	else
		out["category"] = 0;

	return out;
}

void ColorAnalysis::sortMaxima()
{
	//1 or 0 elements --> no sorting needed
	if (hueMaxima.size() < 2)
		return;

	//map sorts automatically
	std::map<float, float, std::greater<float>> tmpMaximaMap;
	std::vector<std::array<float, 2>> newMaxima;
	for (std::array<float, 2> maximaPoint : hueMaxima) {
		tmpMaximaMap.insert(std::pair<float, float>(maximaPoint[1], maximaPoint[0]));
	}

	for (auto maximaPoint : tmpMaximaMap) {
		newMaxima.push_back({maximaPoint.second, maximaPoint.first});
	}
	hueMaxima = newMaxima;
}