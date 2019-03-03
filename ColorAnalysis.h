#pragma once
#include "CImg.h"
#include "json.hpp"

using namespace cimg_library;
using json = nlohmann::json;

class ColorAnalysis
{
private:
	CImg<unsigned char> img;
	/* The image will be scaled down to this resolution, to improve the performance*/
	int downscaleResolution;
	/*
	 * Average saturation and brightness values of the image 
	 */
	float avrSat, avrVal;
	/*
	 * list of all hue maximas; format:
	 *		{
	 *			[x-coordinate/hue, y-coordinate/value],
	 *			...
	 *		}
	 */
	std::vector<std::array<float, 2>> maxima;
	/*
	 * sorts the maxima by value, to get most dominant color/hue first
	 */
	void sortMaxima();
public:
	/*
	 * imgPath: path to image
	 * autoExecute: whether everything should be calculated at initialization
	 */
	ColorAnalysis(const char* imgPath , bool autoExecute, int downscalingResolution = 32);
	/*
	 * calculates all values, searches for maximas and calculate average saturation and value
	 */
	void analyseImg();
	/*
	 * converts rgb array to hsv array (hue: 0 - 360, sat + val: 0.0 - 1.0)
	 */
	std::array<float, 3> rgbToHsv(int* rgbValues);
	/*
	 * returns hue maxima and average saturation and brightnes in a json object
	 */
	json dataSummary();
};

