#pragma once
#include "CImg.h"
#include "json.hpp"

using namespace cimg_library;
using json = nlohmann::json;

class ColorAnalysis
{
private:
	CImg<unsigned char> img;
	int downscaleResolution;
	float avrSat, avrVal;
	std::vector<std::array<float, 2>> maxima;
	void analyseImg();
	void sortMaxima();
public:
	ColorAnalysis(const char* imgPath , bool autoExecute, int downscalingResolution = 32);
	std::array<float, 3> rgbToHsv(int* rgbValues);
	json dataSummary();
};

