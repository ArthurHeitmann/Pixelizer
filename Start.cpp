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
	/*while (true) {
		int imgNum = 0;
		std::cin >> imgNum;
		std::string path = "img/img";
		path += std::to_string(imgNum);
		path += ".bmp";
		ColorAnalysis ca(path.c_str(), true, 20);
		std::cout << "\n\n";
	}*/

	generateColorTable("img", 36);

	/*json j;

	j["img_table"] = json::array();
	

	j["img_table"].push_back(json::object());
	j["img_table"][0]["hue_maximas"] = { {352, 74}, {6, 67} };
	j["img_table"][0]["avr_sat"] = 0.34;
	j["img_table"][0]["avr_val"] = 0.74;

	j["img_table"].push_back(json::object());
	j["img_table"][1]["hue_maximas"] = { {26, 23}, {267, 11} };
	j["img_table"][1]["avr_sat"] = 0.26;
	j["img_table"][1]["avr_val"] = 0.53;

	std::ofstream indexFile;
	indexFile.open("color_table.json");
	indexFile << j.dump(4);
	indexFile.close();*/

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
	}

	std::ofstream colorTableFile;
	colorTableFile.open("color_table.json");
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}