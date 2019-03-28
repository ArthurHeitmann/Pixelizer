#pragma once
#include "CImg.h"
#include "json.hpp"

using namespace cimg_library;
using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;

class ColorAnalysis
{
private:
	CImg<unsigned char> img, cropOriginalImg;
	
	/* The image will be scaled down to this resolution, to improve the performance*/
	int downscaleResolution;
	/*
	 * Average saturation and brightness values of the image 
	 */
	float avrSat, avrVal, avrValScore;
	/*
	 * list of all hue maximas; format:
	 *		{
	 *			[x-coordinate/hue, y-coordinate/value],
	 *			...
	 *		}
	 */
	std::vector<std::array<float, 2>> hueMaxima;
	/*
	 * sorts the maxima by value, to get most dominant color/hue first
	 */
	void sortMaxima();

	float simplifyNumber(float num);
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
	/*
	 * returns hue maxima and average saturation and brightnes in a json object
	 */
	json dataSummary();

	/**
		Returns minimal distance in degrees on a circle, from 2 given angles
	*/
	int colorCircleDistance(int hue1, int hue2);
};

