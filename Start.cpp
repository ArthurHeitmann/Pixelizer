#pragma once
#include "pch.h"
#include "ColorAnalysis.h"
#include <iostream>
#include <string>
#include "json.hpp"
#include <iostream>
#include <fstream>
#include "ColorMatcher.h"

using json = nlohmann::json;

void generateColorTable(std::string path, int fileCount);
void analyzeTable();


int main() {

	//generateColorTable("img/stock_bmp", 13360);
	std::ifstream jsonFile("color_table_13360.json");
	std::string fileContent;
	if (jsonFile.is_open())
	{
		std::string line = "";
		while (std::getline(jsonFile, line)) {
			fileContent += line;
		}
		jsonFile.close();
	}

	ColorMatcher matcher(json::parse(fileContent));
	while (true)
	{
		float h, s, v;
		std::cin >> h;
		std::cin >> s;
		std::cin >> v;

		float hsvs[] = { h, s / 100, v / 100 };
		std::cout << matcher.findImageMatch(hsvs) + 1 << "\n\n";
	}


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
		colorTable["colorTable"][i - 1]["id"] = i - 1;
		colorTable["colorTable"][i - 1]["uses"] = 0;
	}
	std::ofstream colorTableFile;
	colorTableFile.open("color_table_13360.json");
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}
