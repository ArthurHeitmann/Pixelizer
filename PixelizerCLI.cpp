#include "pch.h"
#include "PixelizerCLI.h"
#include <stdlib.h>


PixelizerCLI::PixelizerCLI()
{
	colorTable = nullptr;
	launch();
}

PixelizerCLI::~PixelizerCLI()
{
	delete colorTable;
}

void PixelizerCLI::launch()
{
	while (true)
	{
		clear();
		printLine();
		print(" Welcome to Pixelizer", true);
		printLine();
		printOptionLine(1, "Create color table from image collection", "", 0);
		printOptionLine(2, "Generate pixelized image", colorTable == nullptr ? "Nothing chached yet" : "Cache available", 4);
		printOptionLine(3, "Audio notifications", alert == '\a', 4);
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
		case 3:
		{
			print(" On: 1 | Off: 0: ");
			int alertInp = 1;
			alertInp = getIntegerInput();
			if (alertInp)
				alert = '\a';
			else
				alert = ' ';
			break;
		}
		default:
			break;
		}
	}
}

void PixelizerCLI::generateColorTable()
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

void PixelizerCLI::executeColorTable(std::string imgCollectionPath, int imgCount, int startIndex, std::string outPath)
{
	clear();
	printLine();
	print(" Working...", true);
	print(" [__________________________________________________]");
	print("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

	json colorTable;
	colorTable["colorTable"] = json::array();

	int prevProgress = 0;
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
	std::cout << alert;
}

void PixelizerCLI::generatePixImage()
{
	bool done = false;
	std::string targetPath = "targetImgs/target_0.bmp";
	try
	{
		templateImg = CImg<unsigned char>(targetPath.c_str());
	}
	catch (const std::exception&)
	{
		targetPath = "";
	}
	float scalingFactor = 1;
	std::string colorTablePath = "color_table_50k_v1.0.json";
	std::string imgOutName = "result_tmp";
	int pixelResolution = 75;
	int noRepeatRange = 30;
	int maxExtensions = 15;
	bool useAltAlgo = false;
	bool canStart = false;
	int threads = 8;
	int splitImgs = 1;
	
	while (!done)
	{
		clear();
		printLine();
		print("Find image matches and create final image", true);
		printLine();
		printOptionLine(1, "Path to template image", targetPath, 4);
		if (!templateImg.empty())
		{

			std::string tmpResolution = std::to_string((int) round(templateImg.width() * scalingFactor));
			tmpResolution += " px x ";
			tmpResolution += std::to_string((int) round(templateImg.height() * scalingFactor));
			tmpResolution += " px";
			printOptionLine(2, "target image resolution", tmpResolution, 4);
		}
		printOptionLine(3, "Pixelized image output path (without .bmp)", imgOutName, 2);
		printOptionLine(4, "Path to color table (\".\" for cached file)", colorTablePath, 2);
		printOptionLine(5, "Small images resolution", pixelResolution, 4);
		printOptionLine(6, "Avoid duplicates in range", noRepeatRange, 4);
		printOptionLine(7, "Maximum range extensions", maxExtensions, 4);
		printOptionLine(8, "UseAlternative distance alogrithm", useAltAlgo, 3);
		printOptionLine(9, "Threads", threads, 6);
		printOptionLine(10, "Output image times split", splitImgs, 4);
		printOptionLine(0, "Exit", "", 0);
		printLine();
		if (!targetPath.empty() && scalingFactor > 0 && !colorTablePath.empty() && !imgOutName.empty() && pixelResolution > 0 && noRepeatRange >= 0 && maxExtensions >= 0 && threads > 0) {
			canStart = true;
			printOptionLine(11, "Start", "Ready", 6);
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
				templateImg = CImg<unsigned char>(targetPath.c_str());
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
				print(" Maximum extensions: ");
				maxExtensions = getIntegerInput();
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
				printLine();
				print(" Speperate images: ");
				splitImgs = getIntegerInput();
				break;
			case 11:
				if (canStart) {
					done = true;
					executePixImage(targetPath, scalingFactor, colorTablePath, imgOutName, pixelResolution, noRepeatRange, maxExtensions, useAltAlgo, threads, splitImgs);
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

void PixelizerCLI::executePixImage(std::string targetPath, float scalingFactor, std::string colorTablePath, std::string imgOutName, int pixelResolution,
	int noRepeatRange, int maxExtensions, bool useAltAlgo, int threads, int seperateImgs)
{
	clear();
	printLine();
	print(" init:...");
	Pixelizer pix(targetPath.c_str(), colorTablePath.c_str(), imgOutName.c_str(), scalingFactor, colorTable, threads, pixelResolution, noRepeatRange, maxExtensions, useAltAlgo, seperateImgs);
	if (colorTablePath != ".")
		colorTable = pix.getColorTable();
	printLine();
	print(" matching...", true);
	pix.findImageMatches();
	std::cout << alert;
	printLine();
	print(" finalizing...", true);
	pix.createFinalImg();
}

float PixelizerCLI::getImageScaling()
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
		return (float) getIntegerInput() / templateImg.width();
	case 3:
		print(" Height: ");
		return (float)getIntegerInput() / templateImg.height();
	default:
		return 1;
	}
	

}

void PixelizerCLI::printOptionLine(int key, std::string text, std::string value, int tabCount)
{
	std::cout << " " << key << ": " << text << ":";
	for (int i = 0; i < tabCount; i++)
		std::cout << "\t";
	std::cout << value << "\n";
}

void PixelizerCLI::printOptionLine(int key, std::string text, int value, int tabCount)
{
	std::cout << " " << key << ": " << text << ":";
	for (int i = 0; i < tabCount; i++)
		std::cout << "\t";
	std::cout << value << "\n";
}

void PixelizerCLI::clear()
{
	system("cls");
	std::cout << std::flush;
}

void PixelizerCLI::printLine()
{
	std::cout << "\n";
}

void PixelizerCLI::print(std::string text, bool newLine)
{
	std::cout << text;
	if (newLine)
		std::cout << "\n";
}

int PixelizerCLI::getIntegerInput()
{
	int out;
	std::cin >> out;
	return out;
}

float PixelizerCLI::getFloatInput()
{
	float tmpF;
	std::cin >> tmpF;
	return tmpF;
}

std::string PixelizerCLI::getTextInput()
{
	std::string path;
	std::cin >> path;
	return path;
}
