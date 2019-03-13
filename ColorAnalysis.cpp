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
			img.crop(0, (img.height() - img.width()) / 2, img.width(), (img.width() + (img.height() - img.width()) / 2));
		else
			img.crop((img.width() - img.height()) / 2, 0, (img.height() + (img.width() - img.height()) / 2), img.height());
	}
	cropOriginalImg = CImg<unsigned char>(img);
	cropOriginalImg.crop(0, 0, cropOriginalImg.width() * 2, cropOriginalImg.height());
	img.resize(downscaleResolution, downscaleResolution, 1, 3, 5);
	//img.display();
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
	//intDest.printFunction(2);
	hueMaxima = intDest.findMaxima(0);
	sortMaxima();
	avrSat = intDest.average(1);
	avrVal = intDest.average(2);
	avrValScore =  intDest.calcValueAt(2, avrVal);

	/*std::string path = "out/";
	path += filePath;
	CImg<unsigned char> colorImg(100, 100, 1, 3);*/

	//intDest.printFunction(0);
}



json ColorAnalysis::dataSummary()
{
	json out = json::object();
	
	if (hueMaxima.size() != 0) {
		out["max_hue"] = hueMaxima[0][0];
		out["max_hue_score"] = hueMaxima[0][1];
	}

	out["avr_val_score"] = avrValScore;

	out["avr_sat"] = avrSat;
	out["avr_val"] = avrVal;

		//extremely low sat or brightnes --> hue cannot be seen anymore
	
	if (avrSat < 0.09 || avrVal < 0.06 || hueMaxima.size() == 0)				//grayscale
		out["category"] = 2;
	else if (hueMaxima.size() == 1)
		out["category"] = 0;
	else if (hueMaxima[0][1] > 20 && hueMaxima[0][1] / hueMaxima[1][1] < 3 && colorCircleDistance((int)hueMaxima[0][0], (int)hueMaxima[1][0]) > 40)
		out["category"] = 1;
	else if (hueMaxima[0][1] <= 20 && hueMaxima[0][1] / hueMaxima[1][1] < 2 && colorCircleDistance((int)hueMaxima[0][0], (int)hueMaxima[1][0]) > 40)
		out["category"] = 1;
	else if (hueMaxima.size() > 2 && hueMaxima[2][1] > 4 && hueMaxima[0][1] / hueMaxima[2][1] < 3)
		out["category"] = 1;
	else
		out["category"] = 0;

	/*
	for (int x = cropOriginalImg.width() / 2; x < cropOriginalImg.width(); x++)
	{
		float tmpHsv[] = { (hueMaxima.size() != 0 ? hueMaxima[0][0] : 0), avrSat, avrVal };
		std::array<int, 3> rgbVal = hsvToRgb(tmpHsv);
		for (int y = 0; y < cropOriginalImg.height(); y++)
		{
			if (out["category"] == 1) {
				cropOriginalImg(x, y, 0, 0) = 255;
				cropOriginalImg(x, y, 0, 1) = 0;
				cropOriginalImg(x, y, 0, 2) = 0;
			}
			else
			{
				cropOriginalImg(x, y, 0, 0) = rgbVal[0];
				cropOriginalImg(x, y, 0, 1) = rgbVal[1];
				cropOriginalImg(x, y, 0, 2) = rgbVal[2];
			}
		}
	}
	cropOriginalImg.display();*/


	return out;
}

int ColorAnalysis::colorCircleDistance(int hue1, int hue2)
{
	int minDistance = abs(hue1 - hue2);

	return minDistance > 180 ? 360 - minDistance : minDistance;
}

void ColorAnalysis::sortMaxima()
{
	if (hueMaxima.size() < 2)
		return;

	std::map<float, float, std::greater<float>> tmpMaximaMap;
	std::vector<std::array<float, 2>> newMaxima;
	for (std::array<float, 2> maximaPoint : hueMaxima) {
		tmpMaximaMap.insert(std::pair<float, float>(maximaPoint[1], maximaPoint[0]));
	}

	/*if (tmpMaximaMap.begin()->second < 5)
	{
		hueMaxima.clear();
		hueMaxima.push_back({ tmpMaximaMap.begin()->second, tmpMaximaMap.begin()->first });
		return;
	}*/

	for (auto maximaPoint : tmpMaximaMap) {
		newMaxima.push_back({maximaPoint.second, maximaPoint.first});
	}
		hueMaxima = newMaxima;
}

std::array<int, 3> ColorAnalysis::hsvToRgb(float hsv[3])
{
	
	float tmpRgb[3];
	std::array<int, 3> out;
	int i;
	float f, p, q, t;

	if (hsv[1] < 0.01)
	{
		out = { (int)(hsv[2] * 255), (int)(hsv[2] * 255), (int)(hsv[2] * 255) };
		return out;
	}
	hsv[0] /= 60;
	i = hsv[0];
	f = hsv[0] - i;
	p = hsv[2] * (1 - hsv[1]);
	q = hsv[2] * (1 - hsv[1] * f);
	t = hsv[2] * (1 - hsv[1] * (1 - f));

	switch (i)
	{
	case 0:
		tmpRgb[0] = hsv[2];
		tmpRgb[1] = t;
		tmpRgb[2] = p;
		break;
	case 1:
		tmpRgb[0] = q;
		tmpRgb[1] = hsv[2];
		tmpRgb[2] = p;
		break;
	case 2:
		tmpRgb[0] = p;
		tmpRgb[1] = hsv[2];
		tmpRgb[2] = t;
		break;
	case 3:
		tmpRgb[0] = p;
		tmpRgb[1] = q;
		tmpRgb[2] = hsv[2];
		break;
	case 4:
		tmpRgb[0] = t;
		tmpRgb[1] = p;
		tmpRgb[2] = hsv[2];
		break;
	default:
		tmpRgb[0] = hsv[2];
		tmpRgb[1] = p;
		tmpRgb[2] = q;
		break;
	}

	tmpRgb[0] *= 255;
	tmpRgb[1] *= 255;
	tmpRgb[2] *= 255;

	out = { (int)round(tmpRgb[0]), (int)round(tmpRgb[1]),(int)round(tmpRgb[2]) };

	return out;
}

std::array<float, 3> ColorAnalysis::rgbToHsv(int* rgb)
{
	//straight from so
	std::array<float, 3> hsv;
	int min = rgb[0] < rgb[1] ? rgb[0] : rgb[1];
	min = min < rgb[2] ? min : rgb[2];
	int max = rgb[0] > rgb[1] ? rgb[0] : rgb[1];
	max = max > rgb[2] ? max : rgb[2];

	hsv[2] = (float)max / 255;
	int delta = max - min;
	if (delta == 0)
	{
		hsv[0] = 0;
		hsv[1] = 0;
		return hsv;
	}

	if (max > 0)
	{
		hsv[1] = (float)delta / max;
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
		hsv[0] = 2 + (float)(rgb[2] - rgb[0]) / delta;
	else
		hsv[0] = 4 + (float)(rgb[0] - rgb[1]) / delta;

	hsv[0] *= 60;
	if (hsv[0] < 0)
		hsv[0] += 360;

	return hsv;
}