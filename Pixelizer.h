#pragma once
#include "json.hpp"
#include "CImg.h"
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include "PixTools.h"

using namespace cimg_library;

class Pixelizer
{
private:
	/* holds data of all images from the image library */
	std::vector<imgData>* colorTable;
	/* used for changing the values of "times used" and "used at"*/
	std::vector<std::mutex>* locks;
	/** base multiplier for "maxExtensions"; (multiplier * maxExtensions) --> maximum +/- range from a value */
	float grayscaleRange, hueRange, saturationRange, valueRange, maxExtensions;
	/* range in which an image (optimally) shouldn't be used twice */
	int noRepeatRange;
	/* resolution of the small (squared) images */
	int pixelImgsResolution;
	/* the image which is the template for the final image */
	CImg<unsigned char>* templateImg;
	/* file paths to all small images, that will be on the final image */
	std::vector<std::vector<std::string>>* pixelImgPaths;
	/* file path to the output file (without ".bmp") */
	const char* resultFile;
	/* 
	 whether an alternative algorithm should be used for the distance calculation
	 1. a "grid" will be placed on the template image
	 2. when an image is placed in one of the squares, the coordinates of that square will be stored under "used_at" under "colorTable"
	 3. the same image won't (optimally) be used in that or the 8 neighbour squares
	*/
	bool useAltDistanceAlgo;
	/* the amount of divisions along the x-axis if the image (when using the alternative distance algorithm) */
	int divisions;
	/* the amount of threads used for the current operation (matching or finalizing) */
	int threads;
	/* how many threads have finished the curred operation */
	int threadsDone;
	int splitImagesOutput, splitImgTsDone;
	/* lock for updating the threads done variable */
	std::mutex tsDoneLock;
	/* percentage of the current operation (0 min, 100 max) */
	float progressPercent;
	/* lock for updating the "progressPercent" variable */
	std::mutex progressLock;
	/* cache of the coordinates when using the alt distance algo */
	std::vector<std::vector<std::array<int, 2>>>* altCoordsCache;

	/* searches for all image matches in given x intervall
	 startX: first column that this thread takes care of
	 endX - 1: last column
	*/
	void findImageMatchesThread(int startX, int endX);
	/* displays the progress, of the currently running operation, in the console */
	void progressThread();
	/* assembles an image in the given intervall; isAlong: true --> the final image will be assembled in one piece */
	void creatFinalImgThread(int startX, int endX, int threadNum, bool isAlone);
	/* searches for all images that visually could be a match to the current pixel
	 targetPixel: the hsv pixel to which the image match should be found
	 posXY: X-Y-coordinate of the target pixel

	 return value: element id of the image match in "colorTable"
	*/
	int findImageMatch(std::array<float, 3> targetPixel, int posXY[2]);
	/* From all visually fitting images choose the one that hasn't been used in a close range allready */
	int imgMatchByDistance(const std::vector<std::map<float, int, std::greater<float>>>* const closeImgs, int posXY[2]);
	/* from all fitting images, choose one randomly, whilst prioritising images with a high score (float value in the map) */
	int randomTopImgId(const std::map<float, int, std::greater<float>> &imgs);
	/* returns allternative coordinates */
	std::array<int, 2> getAlternativeCoodrdinates(int* posXY);
	/* returns allternative coordinates */
	std::array<int, 2> getAlternativeCoodrdinates(std::array<int, 2> posXY);
public:
	Pixelizer(const char* targetImgPath, const  char* colorTablePath, const  char* resultFile, float targetImgScalingFactor, std::vector<imgData>* colorTable = nullptr,
		int threads = 8, int pixelImgsResolution = 150, float noRepeatRange = 7, int maxExtensions = 15, bool altDistance = false, int splitImages = 1);
	~Pixelizer();
	/* finds all image matches to the template image */
	void findImageMatches();
	/* assembles the final output image(s) and saves it/them */
	void createFinalImg();
	/* returns the colorTable (for caching, since reading and parsing takes about 45 seconds ) */
	std::vector<imgData>* getColorTable();

};

