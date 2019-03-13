#pragma once
#include "json.hpp"

using json = nlohmann::json;

class ColorMatcher
{
private:
	json colorTable;
	float greyscaleRange, hueRange, saturationRange, valueRange;
	float distanceOnCircle(float val1, float val2);
public:
	ColorMatcher(json colorTable, float greyscaleRange = 0.06, float hueRange = 5, float saturationRange = 0.07, float valueRange = 0.07);
	int findImageMatch(float hsvPixels[3]);
};

