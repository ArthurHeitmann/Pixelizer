#pragma once
#include "pch.h"
#include "ColorAnalysis.h"
#include <iostream>
#include <string>
#include "json.hpp"
#include <iostream>
#include <fstream>

using json = nlohmann::json;

void generateColorTable(std::string path, int fileCount);

int main() {

	generateColorTable("img", 36);

	return 0;
}

void generateColorTable(std::string path, int fileCount) {
	int test;
	std::cin >> test;
	json colorTable;
	colorTable["colorTable"] = json::array();

	for (int i = 1; i <= fileCount; i++)
	{
		std::string filePath = path + "/img (";
		filePath += std::to_string(i);
		filePath += ").bmp";

		ColorAnalysis ca(filePath.c_str(), true, 20);

		colorTable["colorTable"].push_back(ca.dataSummary());
		colorTable["colorTable"][i - 1]["file_path"] = filePath;
		colorTable["colorTable"][i - 1]["id"] = i - 1;
	}

	std::ofstream colorTableFile;
	colorTableFile.open("color_table.json");
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}