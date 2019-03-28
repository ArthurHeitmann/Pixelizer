#pragma once
#include <iostream>
#include "Pixelizer.h"
#include <fstream>
#include <string>
#include "ColorAnalysis.h"
#include "CImg.h"

using namespace cimg_library;

class PixelizerGUI
{
private:
	CImg<unsigned char> targetImg;
	int completionStatus[2];

	void launch();
	void generateColorTable();
	void executeColorTable(std::string imgCollectionPath, int imgCount,	int startIndex, std::string outPath);
	void generatePixImage();
	void executePixImage(std::string targetPath, float scalingFactor, std::string colorTablePath, std::string imgOutName, int pixelResolution, int noRepeatRange, int maxRecursion, bool useAltAlgo, int threads);

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
	PixelizerGUI();
};

