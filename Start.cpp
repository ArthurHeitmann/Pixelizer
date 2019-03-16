#pragma once
#include "pch.h"
#include "ColorAnalysis.h"
#include <iostream>
#include <string>
#include "json.hpp"
#include <iostream>
#include <fstream>
#include "Pixelizer.h"

using json = nlohmann::json;
using namespace cimg_library;

void generateColorTable(std::string path, int fileCount);


int main() {
	//generateColorTable("img/stock_bmp", 13360);

	std::cout << "init: ...";
	Pixelizer pix("target_5.bmp", "color_table_v1.json", 1, 45, 60, 0.04, 8, 0.085, 0.085);
	std::cout << "matching...\n";
	pix.findImageMatches();
	std::cout << "finalizing\n";
	pix.createFinalImg();


	return 0;
}

void generateColorTable(std::string path, int fileCount) {
	json colorTable;
	colorTable["colorTable"] = json::array();

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
	}
	std::ofstream colorTableFile;
	colorTableFile.open("color_table_v1.json");
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}
