#include "pch.h"
#include "ColorMatcher.h"
#include <vector>
#include <iostream>

float ColorMatcher::distanceOnCircle(float val1, float val2)
{
	int minDistance = abs(val1 - val2);

	return minDistance > 180 ? 360 - minDistance : minDistance;
}

ColorMatcher::ColorMatcher(json colorTable, float greyscaleRange, float hueRange, float saturationRange, float valueRange)
{
	this->colorTable = colorTable;
	this->greyscaleRange = greyscaleRange;
	this->hueRange = hueRange;
	this->saturationRange = saturationRange;
	this->valueRange = valueRange;
}

int ColorMatcher::findImageMatch(float hsvPixels[3])
{
	std::vector<int> closeImgs;

	//greyscale
	if (hsvPixels[1] < 0.09 || hsvPixels[2] < 0.04)
	{
		//find all images withing range
		for (int i = 0; i < colorTable["colorTable"].size(); i++)
		{
			if (colorTable["colorTable"][i]["category"] == 2 && 
				abs(hsvPixels[2] - (float) colorTable["colorTable"][i]["avr_val"]) < greyscaleRange)
				closeImgs.push_back(i);
		}

		//extend range if no images were found
		if (closeImgs.empty()) {
			this->greyscaleRange *= 1.25;
			std::cout << "extending grey to " << greyscaleRange << std::endl;
			int out = findImageMatch(hsvPixels);
			this->greyscaleRange /= 1.25;
			return out;
		}

		//find image where the average Value appears the most often
		std::map<float, float, std::greater<float>> valueScoreSort;
		for (int id : closeImgs)
		{
			valueScoreSort.insert(std::pair<float, float>(colorTable["colorTable"][id]["avr_val_score"], id));
		}
		
		return valueScoreSort.begin()->second;
	}
	//colorful pixel :)
	else
	{
		//find all images withing range
		for (int i = 0; i < colorTable["colorTable"].size(); i++)
		{
			if (colorTable["colorTable"][i]["category"] == 0)
			{
				if (distanceOnCircle(hsvPixels[0], colorTable["colorTable"][i]["max_hue"]) < hueRange &&
					abs(hsvPixels[1] - (float) colorTable["colorTable"][i]["avr_sat"]) < saturationRange &&
					abs(hsvPixels[2] - (float) colorTable["colorTable"][i]["avr_val"]) < valueRange)
					closeImgs.push_back(i);
			}
		}

		//extend range if no images were found
		if (closeImgs.empty()) {
			this->hueRange *= 1.25;
			this->saturationRange *= 1.65;
			this->valueRange *= 1.65;
			std::cout << "extending hue to " << hueRange << std::endl;
			int out = findImageMatch(hsvPixels);
			this->hueRange /= 1.25;
			this->saturationRange /= 1.65;
			this->valueRange /= 1.65;
			return out;
		}

		//find image where the target hue appears the most often
		std::map<float, float, std::greater<float>> hueScoreSort;
		for (int id : closeImgs)
		{
			hueScoreSort.insert(std::pair<float, float>(colorTable["colorTable"][id]["max_hue_score"], id));
		}

		return hueScoreSort.begin()->second;
	}



	return 0;
}
