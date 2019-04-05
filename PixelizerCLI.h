#pragma once
#include <iostream>
#include "Pixelizer.h"
#include <fstream>
#include <string>
#include "ColorAnalysis.h"
#include "CImg.h"
#include <vector>
#include "PixTools.h"

using namespace cimg_library;

class PixelizerCLI
{
private:
	CImg<unsigned char> templateImg;
	int completionStatus[2];
	char alert = '\a';
	std::vector<imgData>* colorTable;

	void launch();
	void generateColorTable();
	void executeColorTable(std::string imgCollectionPath, int imgCount,	int startIndex, std::string outPath);
	void generatePixImage();
	void executePixImage(std::string targetPath, float scalingFactor, std::string colorTablePath, std::string imgOutName, int pixelResolution,
		int noRepeatRange, int maxExtensions, bool useAltAlgo, int threads, int speperateImgs);

	float getImageScaling();

	void printOptionLine(int key, std::string text, std::string value, int tabCount);
	void printOptionLine(int key, std::string text, int value, int tabCount);
	void clear();
	void printLine();
	void print(std::string text, bool newLine = false);
	int getIntegerInput();
	float getFloatInput();
	std::string getTextInput();
	
public:
	PixelizerCLI();
	~PixelizerCLI();
};

