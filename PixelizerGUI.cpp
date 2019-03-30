#include "pch.h"
#include "PixelizerGUI.h"
#include <stdlib.h>


PixelizerGUI::PixelizerGUI()
{
	launch();
}

void PixelizerGUI::launch()
{
	while (true)
	{
		clear();
		printLine();
		print(" Welcome to Pixelizer", true);
		printLine();
		printOptionLine(1, "Create color table from image collection", "", 0);
		printOptionLine(2, "Generate pixelized image", "", 0);
		printOptionLine(0, "Exit", "", 0);
		printLine();
		printLine();
		print(" Input: ");
		switch (getIntegerInput())
		{
		case 0:
			exit(0);
		case 1:
			generateColorTable();
			break;
		case 2:
			generatePixImage();
			break;
		default:
			break;
		}
	}
}

void PixelizerGUI::generateColorTable()
{
	bool done = false;
	std::string imgCollectionPath = "img/stock_bmp/img_";
	int imgCount = 80;
	int startIndex = 1;
	std::string outPath = "color_table.json";
	bool canStart = false;

	while (!done)
	{
		clear();
		printLine();
		print("Find image matches and create final image", true);
		printLine();
		printOptionLine(1, "Path to .bmp image collection (with file prefix)", (imgCollectionPath.empty() ? "undefined" : imgCollectionPath), 1);
		printOptionLine(2, "Number of images in the folder", (imgCount == -1 ? "undefined" : std::to_string(imgCount)), 3);
		printOptionLine(3, "Index of first image", startIndex, 4);
		printOptionLine(4, "Output file path", outPath, 5);
		printOptionLine(0, "Exit", "", 0);
		printLine();
		if (!imgCollectionPath.empty() && imgCount > 0 && !outPath.empty()) {
			canStart = true;
			printOptionLine(5, "Start", "", 0);
		}
		printLine();
		printLine();
		print(" Input: ");
		switch (getIntegerInput())
		{
		case 0:
			exit(0);
		case 1:
			printLine();
			print(" Path to image collection: ");
			imgCollectionPath = getTextInput();
			break;
		case 2:
			printLine();
			print(" Image count: ");
			imgCount = getIntegerInput();
			break;
		case 3:
			printLine();
			print( "First index: ");
			startIndex = getIntegerInput();
			break;
		case 4:
			printLine();
			print(" Output file path: ");
			outPath = getTextInput();
			break;
		case 5:
			if (canStart) {
				done = true;
				executeColorTable(imgCollectionPath, imgCount, startIndex, outPath);
				break;
			}
			else
				break;
		default:
			break;
		}
	}
}

void PixelizerGUI::executeColorTable(std::string imgCollectionPath, int imgCount, int startIndex, std::string outPath)
{
	clear();
	printLine();
	print(" Working...", true);
	print(" [__________________________________________________]");
	print("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

	json colorTable;
	colorTable["colorTable"] = json::array();

	int prevProgress = -1;
	bool prevWasHalf = false;;
	for (int i = 1; i <= imgCount; i++)
	{
		/*int test;
		std::cin >> test;
		*/
		std::string filePath = imgCollectionPath;
		filePath += std::to_string(i);
		filePath += ".bmp";

		ColorAnalysis ca(filePath.c_str(), true, 20);

		colorTable["colorTable"].push_back(ca.dataSummary());
		colorTable["colorTable"][i - 1]["file_path"] = filePath;

		if ((int)((100 * i) / imgCount) > prevProgress)
		{
			print((prevWasHalf ? "=" : "-\b"));
			prevWasHalf = !prevWasHalf;
			prevProgress++;
		}
	}
	std::ofstream colorTableFile;
	colorTableFile.open(outPath);
	colorTableFile << colorTable.dump(4);
	colorTableFile.close();
}

void PixelizerGUI::generatePixImage()
{
	bool done = false;
	std::string targetPath = "targetImgs/target_7.bmp";
	try
	{
		targetImg = CImg<unsigned char>(targetPath.c_str());
	}
	catch (const std::exception&)
	{
		targetPath = "";
	}
	float scalingFactor = 1;
	std::string colorTablePath = "color_table_v2.2.json";
	std::string imgOutName = "result_tmp.bmp";
	int pixelResolution = 75;
	int noRepeatRange = 15;
	int maxRecursion = 15;
	bool useAltAlgo = false;
	bool canStart = false;
	int threads = 8;
	
	while (!done)
	{
		clear();
		printLine();
		print("Find image matches and create final image", true);
		printLine();
		printOptionLine(1, "Path to template image", targetPath, 3);
		if (!targetImg.empty())
		{

			std::string tmpResolution = std::to_string((int) round(targetImg.width() * scalingFactor));
			tmpResolution += " px x ";
			tmpResolution += std::to_string((int) round(targetImg.height() * scalingFactor));
			tmpResolution += " px";
			printOptionLine(2, "target image resolution", tmpResolution, 3);
		}
		printOptionLine(3, "Pixelized image output path (.bmp)", imgOutName, 2);
		printOptionLine(4, "File path of the color table (.json)", colorTablePath, 1);
		printOptionLine(5, "Small images resolution", pixelResolution, 3);
		printOptionLine(6, "Avoid duplicates in range", noRepeatRange, 3);
		printOptionLine(7, "Maximum recursion depth", maxRecursion, 3);
		printOptionLine(8, "UseAlternative distance alogrithm", useAltAlgo, 2);
		printOptionLine(9, "Threads", threads, 5);
		printOptionLine(0, "Exit", "", 0);
		printLine();
		if (!targetPath.empty() && scalingFactor > 0 && !colorTablePath.empty() && !imgOutName.empty() && pixelResolution > 0 && noRepeatRange >= 0 && maxRecursion >= 0 && threads > 0) {
			canStart = true;
			printOptionLine(10, "Start", "", 0);
		}
		else
			canStart = false;
		printLine();
		printLine();
		print(" Input: ");
		try
		{
			switch (getIntegerInput())
			{
			case 0:
				exit(0);
			case 1:
				printLine();
				print(" Template image path: ");
				targetPath = getTextInput();
				targetImg = CImg<unsigned char>(targetPath.c_str());
				break;
			case 2:
				printLine();
				print(" Image count: ");
				scalingFactor = getImageScaling();
				break;
			case 3:
				printLine();
				print("Output path: ");
				imgOutName = getTextInput();
				/*if (imgOutName.find(".bmp") == imgOutName.size() - 4)
					throw std::exception("invalid file name (has to end with .bmp)");*/
				break;
			case 4:
				printLine();
				print(" Path to color table: ");
				colorTablePath = getTextInput();
				break;
			case 5:
				printLine();
				print(" Pixel images resolution: ");
				pixelResolution = getIntegerInput();
				break;
			case 6:
				printLine();
				print(" No repeat range: ");
				noRepeatRange = getIntegerInput();
				break;
			case 7:
				printLine();
				print(" Maximum recursion: ");
				maxRecursion = getIntegerInput();
				break;
			case 8:
				printLine();
				print(" Use alt algo (1: yes 0: no): ");
				useAltAlgo = getIntegerInput();
				break;
			case 9:
				printLine();
				print(" Threads: ");
				threads = getIntegerInput();
				break;
			case 10:
				if (canStart) {
					done = true;
					executePixImage(targetPath, scalingFactor, colorTablePath, imgOutName, pixelResolution, noRepeatRange, maxRecursion, useAltAlgo, threads);
					break;
				}
				else
					break;
			default:
				break;
			}
		}
		catch (const std::exception& e)
		{
			print("Invalid input. Please try again (");
			print(e.what());
			print(")");
			Sleep(6000);
		}
	}
}

void PixelizerGUI::executePixImage(std::string targetPath, float scalingFactor, std::string colorTablePath, std::string imgOutName, int pixelResolution, int noRepeatRange, int maxRecursion, bool useAltAlgo, int threads)
{
	clear();
	printLine();
	print(" init:...");
	Pixelizer pix(targetPath.c_str(), colorTablePath.c_str(), imgOutName.c_str(), scalingFactor, threads, pixelResolution, noRepeatRange, maxRecursion, useAltAlgo);
	printLine();
	print(" matching...", true);
	pix.findImageMatches();
	std::cout << "\a";
	printLine();
	print(" finalizing...", true);
	pix.createFinalImg();
}

float PixelizerGUI::getImageScaling()
{
	clear();
	printLine();
	print(" How do you want to set the resolution?");
	printLine();
	printOptionLine(1, "Set scaling factor", "", 0);
	printOptionLine(2, "Set width", "", 0);
	printOptionLine(3, "Set height", "", 0);
	printOptionLine(0, "cancel", "", 0);
	printLine();
	print(" Input: ");
	switch (getIntegerInput())
	{
	case 0:
		break;
	case 1:
		print(" Scaling factor: ");
		return getFloatInput();
	case 2:
		print(" Width: ");
		return (float) getIntegerInput() / targetImg.width();
	case 3:
		print(" Height: ");
		return (float)getIntegerInput() / targetImg.height();
	default:
		return 1;
	}
	

}

void PixelizerGUI::printOptionLine(int key, std::string text, std::string value, int tabCount)
{
	std::cout << " " << key << ": " << text << ":";
	for (int i = 0; i < tabCount; i++)
		std::cout << "\t";
	std::cout << value << "\n";
}

void PixelizerGUI::printOptionLine(int key, std::string text, int value, int tabCount)
{
	std::cout << " " << key << ": " << text << ":";
	for (int i = 0; i < tabCount; i++)
		std::cout << "\t";
	std::cout << value << "\n";
}

void PixelizerGUI::clear()
{
	system("cls");
	std::cout << std::flush;
}

void PixelizerGUI::printLine()
{
	std::cout << "\n";
}

void PixelizerGUI::print(std::string text, bool newLine)
{
	std::cout << text;
	if (newLine)
		std::cout << "\n";
}

int PixelizerGUI::getIntegerInput()
{
	int out;
	std::cin >> out;
	return out;
}

float PixelizerGUI::getFloatInput()
{
	float tmpF;
	std::cin >> tmpF;
	return tmpF;
}

std::string PixelizerGUI::getTextInput()
{
	std::string path;
	std::cin >> path;
	return path;
}
