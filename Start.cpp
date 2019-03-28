#pragma once
#include "pch.h"
#include "ColorAnalysis.h"
#include <iostream>
#include <string>
#include "json.hpp"
#include <fstream>
#include "Pixelizer.h"
#include "PixelizerGUI.h"

using namespace cimg_library;

void generateColorTable(std::string path, int fileCount);

float simplifyNumber(float num)
{
	if (num < 0.01)
		return 0;

	return (int)(num * 1000) / 1000.f;
}


int main() {
	new PixelizerGUI();
	//generateColorTable("img/stock_bmp", 13360);
	std::cout << "init: ...";
	Pixelizer pix("target_7.bmp", "color_table_v1.json", "hummingbird2.bmp", .85, 50, 25);
	std::cout << "matching...\n";
	pix.findImageMatches();
	std::cout << "finalizing\n";
	pix.createFinalImg();
	
	return 0;


}

void generateColorTable(std::string path, int fileCount) {
	using json = nlohmann::json;
	json colorTable;
	colorTable["colorTable"] = json::array();

	int prevProgress = -1;
	for (int i = 0; i < 100; i++)
		std::cout << "|";
	std::cout << "\n";
	for (int i = 1; i <= fileCount; i++)
	{
		/*int test;
		std::cin >> test;
		*/
		std::string filePath = path + "/img_";
		filePath += std::to_string(i);
		filePath += ".bmp";

		ColorAnalysis ca(filePath.c_str(), true, 20);

		colorTable["colorTable"].push_back(ca.dataSummary());
		colorTable["colorTable"][i - 1]["file_path"] = filePath;

		if ((int)((100 * i) / fileCount) > prevProgress)
		{
			std::cout << "|";
			prevProgress++;
		}
	}
	std::ofstream colorTableFile;
	colorTableFile.open("color_table_v2.json");
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}
