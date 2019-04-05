#include "pch.h"
#include "Pixelizer.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <ctime>
#include <cstdlib>

Pixelizer::Pixelizer(const char* targetImgPath, const  char* colorTablePath, const  char* resultFile, float targetImgScalingFactor, std::vector<imgData>* colorTable,
	int threads, int pixelImgsResolution, float noRepeatRange, int maxExtensions, bool altDistance, int splitImages)
{
	this->grayscaleRange = 0.02;
	this->hueRange = 5;
	this->saturationRange = 0.02;
	this->valueRange = 0.02;
	this->resultFile = resultFile;
	this->noRepeatRange = noRepeatRange * noRepeatRange;
	this->pixelImgsResolution = pixelImgsResolution;
	this->maxExtensions = maxExtensions;
	this->useAltDistanceAlgo = altDistance;
	this->threads = threads;
	splitImagesOutput = splitImages;

	pixelImgPaths = new std::vector<std::vector<std::string>>;
	srand(time(NULL));

	templateImg = new CImg<unsigned char>(targetImgPath);
	templateImg->resize((int)templateImg->width() * targetImgScalingFactor, (int)templateImg->height() * targetImgScalingFactor, 1, 3, 5);

	if (useAltDistanceAlgo)
	{
		divisions = (int) (std::max(templateImg->width(), templateImg->height()) / noRepeatRange * 1.5);
		altCoordsCache = new std::vector<std::vector<std::array<int, 2>>>(templateImg->height());
		for (int y = 0; y < templateImg->height(); y++)
		{
			(*altCoordsCache)[y].resize(templateImg->width());
			for (int x = 0; x < templateImg->width(); x++)
			{
				(*altCoordsCache)[y][x] = getAlternativeCoodrdinates({ x, y });
			}
		}
	}

	if (colorTable != nullptr)
	{
		this->colorTable = colorTable;
		for (int i = 0; i < this->colorTable->size(); i++)
		{
			(*colorTable)[i].times_used = 0;
			(*colorTable)[i].used_at = std::vector<std::array<int, 2>>();
		}
	}
	else
	{
		this->colorTable = new std::vector<imgData>;


		std::cout << "reading file...";
		std::ifstream jsonFile(colorTablePath);
		std::string fileContent;
		if (jsonFile.is_open())
		{
			std::string line = "";
			while (std::getline(jsonFile, line)) {
				fileContent += line;
			}
			jsonFile.close();
		}

		std::cout << "parsing json...";
		nlohmann::json tmpJson = nlohmann::json::parse(fileContent);
		std::cout << "converting json...";
		for (int i = 0; i < tmpJson["colorTable"].size(); i++)
		{
			if (tmpJson["colorTable"][i]["category"] == 1)
				continue;
			imgData tmpData;
			tmpData.avr_sat = tmpJson["colorTable"][i]["avr_sat"];
			tmpData.avr_val = tmpJson["colorTable"][i]["avr_val"];
			tmpData.avr_val_score = tmpJson["colorTable"][i]["avr_val_score"];
			try
			{
				tmpData.max_hue = tmpJson["colorTable"][i]["max_hue"];
				tmpData.max_hue_score = tmpJson["colorTable"][i]["max_hue_score"];
			}
			catch (nlohmann::detail::type_error e)
			{
				tmpData.max_hue = 0;
				tmpData.max_hue_score = 0;
			}
			tmpData.category = tmpJson["colorTable"][i]["category"];
			tmpData.file_path = tmpJson["colorTable"][i]["file_path"].get<std::string>();

			this->colorTable->push_back(tmpData);
		}
	}
	locks = new std::vector<std::mutex>(this->colorTable->size());
	std::cout << std::endl;
}
 
Pixelizer::~Pixelizer()
{
	if (useAltDistanceAlgo)
		delete altCoordsCache;
	delete templateImg;
	delete pixelImgPaths;
}

void Pixelizer::findImageMatches()
{
	//resize 2d image paths vector
	pixelImgPaths->resize(templateImg->height());
	for (int y = 0; y < templateImg->height(); y++)
		(*pixelImgPaths)[y].resize(templateImg->width());
	
	progressPercent = 0;
	threadsDone = 0;

	std::vector<std::thread> ts(threads);
	int threadWidths = templateImg->width() / threads;

	std::thread progressT(&Pixelizer::progressThread, this);
	for (int i = 0; i < threads - 1; i++)
	{
		ts[i] = std::thread(&Pixelizer::findImageMatchesThread, this, threadWidths * i, threadWidths * (i + 1));
	}
	ts[threads - 1] = std::thread(&Pixelizer::findImageMatchesThread, this, threadWidths * (threads - 1), templateImg->width());
	for (int i = 0; i < threads; i++)
	{
		ts[i].join();
	}
	progressT.join();
	std::cout << std::endl;
}

void Pixelizer::findImageMatchesThread(int startX, int endX)
{
	float prevProgress = 0;
	int totalPixels = templateImg->height() * templateImg->width();
	for (int y = 0; y < templateImg->height(); y++)
	{
		//update progress every 2 lines (to reduce the amount of times threads get stuck and wait here)
		if (y % 2 == 0)
		{
			float newProgress = (100.f * y * (endX - startX)) / totalPixels;
			progressLock.lock();
			progressPercent += newProgress - prevProgress;
			progressLock.unlock();
			prevProgress = newProgress;
		}
		for (int x = startX; x < endX; x++)
		{
			int posXY[2] = { x, y };
			int tmpRGB[3] = { (*templateImg)(x, y, 0, 0), (*templateImg)(x, y, 0, 1), (*templateImg)(x, y, 0, 2) };
			(*pixelImgPaths)[y][x] = (*colorTable)[findImageMatch(PixTools::rgbToHsv(tmpRGB), posXY)].file_path;
		}
		
	}
	float newProgress = (100.f * templateImg->height() * (endX - startX)) / totalPixels;
	progressLock.lock();
	progressPercent += newProgress - prevProgress;
	progressLock.unlock();

	tsDoneLock.lock();
	threadsDone++;
	tsDoneLock.unlock();
}

void Pixelizer::progressThread()
{
	float prevProgress = 0;
	std::cout << " [__________________________________________________]";
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

	while (threadsDone != threads)
	{
		Sleep(250);
		if ((int) progressPercent > (int) prevProgress) {
			progressLock.lock();
			float progDiff = (int) progressPercent - (int) prevProgress;
			for (int i = 0; i < progDiff; i++)
				std::cout << ((int) (progressPercent - (progDiff - 1 - i)) % 2 == 0 ? "=" : "-\b");
			prevProgress = progressPercent;
			progressLock.unlock();
		}
	}
}

void Pixelizer::creatFinalImgThread(int startX, int endX, int threadNum, bool isAlone)
{
	CImg<unsigned char>* imgPart = new CImg<unsigned char>(pixelImgsResolution * (endX - startX), pixelImgsResolution * templateImg->height(), 1, 3, 0);
	float prevProgress = 0;
	int totalPixels = templateImg->height() * templateImg->width();

	for (int y = 0; y < templateImg->height(); y++)
	{
		//update progress every 2 lines (to reduce the amount of times threads get stuck and wait here)
		if (y % 2 == 0)
		{
			float newProgress = (100.f * y * (endX - startX)) / totalPixels;
			progressLock.lock();
			progressPercent += newProgress - prevProgress;
			progressLock.unlock();
			prevProgress = newProgress;
		}
		for (int x = startX; x < endX; x++)
		{
			CImg<unsigned char> tmpImg((*pixelImgPaths)[y][x].c_str());
			if (tmpImg.width() != tmpImg.height()) {
				if (tmpImg.height() > tmpImg.width())
					tmpImg.crop(0, (tmpImg.height() - tmpImg.width()) / 2, tmpImg.width(), (tmpImg.width() + (tmpImg.height() - tmpImg.width()) / 2));
				else
					tmpImg.crop((tmpImg.width() - tmpImg.height()) / 2, 0, (tmpImg.height() + (tmpImg.width() - tmpImg.height()) / 2), tmpImg.height());
			}
			tmpImg.resize(pixelImgsResolution, pixelImgsResolution, 1, 3, 5);
			imgPart->draw_image((int) (x - startX) * pixelImgsResolution, (int) y * pixelImgsResolution, 0, 0, tmpImg);
		}
	}
	float newProgress = (100.f * templateImg->height() * (endX - startX)) / totalPixels;
	progressLock.lock();
	progressPercent += newProgress - prevProgress;
	progressLock.unlock();

	tsDoneLock.lock();
	threadsDone++;
	tsDoneLock.unlock();
	std::string outputPath = resultFile;
	if (!isAlone)
	{
		outputPath += "_part";
		outputPath += std::to_string(threadNum);
	}
	outputPath += ".bmp";
	imgPart->save(outputPath.c_str());
}

void Pixelizer::createFinalImg()
{
	progressPercent = 0;
	threadsDone = 0;
	threads = splitImagesOutput;

	std::vector<std::thread> ts(splitImagesOutput);
	int threadWidths = templateImg->width() / splitImagesOutput;

	std::thread progressT(&Pixelizer::progressThread, this);
	for (int i = 0; i < splitImagesOutput - 1; i++)
	{
		ts[i] = std::thread(&Pixelizer::creatFinalImgThread, this, threadWidths * i, threadWidths * (i + 1), i, false);
	}
	ts[splitImagesOutput - 1] = std::thread(&Pixelizer::creatFinalImgThread, this, threadWidths * (splitImagesOutput - 1), templateImg->width(), splitImagesOutput - 1, splitImagesOutput == 1);
	for (int i = 0; i < splitImagesOutput; i++)
	{
		ts[i].join();
	}
	progressT.join();
	std::cout << std::endl;
}

std::vector<imgData>* Pixelizer::getColorTable()
{
	return colorTable;
}

int Pixelizer::findImageMatch(std::array<float, 3> targetPixel, int posXY[2])
{
	//all images that from their hsv values could be a match
	//the smaller the index in the vector, the better is the match
	//all images inside the maps are sorted by the amount of occurances of it's main color; So, the higher that float value, the better the image as a possible match
	std::vector<std::map<float, int, std::greater<float>>>* closeImgs = new std::vector<std::map<float, int, std::greater<float>>>(maxExtensions + 1);
	int returnVal = 0;
	//grayscale
	if (targetPixel[1] < 0.025 || targetPixel[2] < 0.03)
	{
		//find all images withing range
		for (int i = 0; i < colorTable->size(); i++)
		{
			if ((*colorTable)[i].category != 2)
				continue;
			int extensionsNeeded = abs(targetPixel[2] - (*colorTable)[i].avr_val) / grayscaleRange;
			if (extensionsNeeded <= maxExtensions)
				(*closeImgs)[extensionsNeeded].insert(std::pair<float, int>((*colorTable)[i].avr_val_score, i));
		}

		//now pick the best image, based on if it has already used close to the target pixel
		if (!closeImgs->empty())
			returnVal =  imgMatchByDistance(closeImgs, posXY);
		
	}
	//colorful pixel :)
	else
	{
		//the hues 60° (yellow), 180° (cyan) and 300° (pink) don't occupy a large arrea on the color circle (on modern monitors), so these areas need to be treated differently
		//(unless my eyes are different than others)
		bool isInThinHueRange = PixTools::distanceOnCircle(targetPixel[0], 60) <= 11 && targetPixel[0] - 60 <= 8 ||
			PixTools::distanceOnCircle(targetPixel[0], 180) <= 25 ||
			PixTools::distanceOnCircle(targetPixel[0], 300) <= 30;
		//find all images within range
		for (int i = 0; i < colorTable->size(); i++)
		{
			if ((*colorTable)[i].category != 0)
				continue;

			int satExtensionCount = abs(targetPixel[1] - (float) (*colorTable)[i].avr_sat) / saturationRange;
			int valExtensionCount = abs(targetPixel[2] - (float) (*colorTable)[i].avr_val) / valueRange;
			if (satExtensionCount > maxExtensions || valExtensionCount > maxExtensions)
				continue;

			int hueDiff = PixTools::distanceOnCircle((*colorTable)[i].max_hue, targetPixel[0]);
			int hueDiffAbs = abs(targetPixel[0] - (*colorTable)[i].max_hue);
			
			int hueExtensionCount;
			if (isInThinHueRange)
			{
				hueExtensionCount = abs(1.5 * hueDiff / hueRange);
				if (hueExtensionCount > maxExtensions)
					continue;
			}
			else
			{
				hueExtensionCount = hueDiff / hueRange;
				if (hueExtensionCount > maxExtensions)
					continue;
				bool skipImage = false;
				for (int edgeCase = 60; edgeCase <= 300 && !skipImage; edgeCase += 120)
				{
					if (hueDiffAbs <= 180)
					{
						if ((edgeCase - (*colorTable)[i].max_hue) * (edgeCase - targetPixel[0]) < 0)
							skipImage = true;
					}
					else
					{
						int tmpMinHue = 360 + std::min(targetPixel[0], (*colorTable)[i].max_hue);
						int tmpMaxHue = std::max(targetPixel[0], (*colorTable)[i].max_hue);
						if ((edgeCase - tmpMinHue) * (edgeCase - tmpMaxHue) < 0 ||
							(edgeCase + 360 - tmpMinHue)* (edgeCase + 360 - tmpMaxHue) < 0)
							skipImage = true;
					}
				}
				if (skipImage)
					continue;

			}
			int maxExt = hueExtensionCount > satExtensionCount ? hueExtensionCount : satExtensionCount;
			maxExt = maxExt > valExtensionCount ? maxExt : valExtensionCount;
			if (maxExt <= maxExtensions)
				(*closeImgs)[maxExt].insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));
		}

		//find image where the target hue appears the most often
		if (!closeImgs->empty())
			returnVal = imgMatchByDistance(closeImgs, posXY);
	}

	delete closeImgs;
	return returnVal;
}

int Pixelizer::imgMatchByDistance(const std::vector<std::map<float, int, std::greater<float>>>* const closeImgs, int posXY[2])
{
	if (posXY[0] == 29 && posXY[1] == 3)
		std::cout << "";
	int firstUsableAt = -1;
	//start with the best matches and check if they are usable, otherwise go to the worse ones, until a good image is found
	for (int extDepth = 0; extDepth < closeImgs->size(); extDepth++)
	{
		//all images that haven't been used close to the target pixel
		std::map<float, int, std::greater<float>> possibleImgs;
		for (std::pair<float, int> score : (*closeImgs)[extDepth])
		{
			if ((*colorTable)[score.second].times_used == 0)
			{
				(*locks)[score.second].lock();
				(*colorTable)[score.second].times_used += 1;
				if (useAltDistanceAlgo)
					(*colorTable)[score.second].used_at.push_back((*altCoordsCache)[posXY[1]][posXY[0]]);
				else
					(*colorTable)[score.second].used_at.push_back({ posXY[0], posXY[1] });
				(*locks)[score.second].unlock();
				return score.second;
			}

			//check for distance to same picture
			bool imgUsable = true;
			for (int i = 0; i < (*colorTable)[score.second].times_used && imgUsable; i++)
			{
				if (useAltDistanceAlgo)
				{
					if (abs((*altCoordsCache)[posXY[1]][posXY[0]][0] - (*colorTable)[score.second].used_at[i][0]) <= 1 &&
						abs((*altCoordsCache)[posXY[1]][posXY[0]][1] - (*colorTable)[score.second].used_at[i][1]) <= 1)
					{
						imgUsable = false;
					}
				}
				else
				{
					//determine distance through the Pythagorean theorem (no repeat range has been squared in the constructor, so that the sqrt() isn't needed anymore)
					if (pow(posXY[0] - (*colorTable)[score.second].used_at[i][0], 2) +
						pow(posXY[1] - (*colorTable)[score.second].used_at[i][1], 2)
						<= noRepeatRange)
					{
						imgUsable = false;
					}
				}
			}

			if (imgUsable) {
				possibleImgs.insert(score);
				if (firstUsableAt == -1)
					firstUsableAt = extDepth;
			}
		}

		if (!possibleImgs.empty() && (firstUsableAt + 2 == extDepth || extDepth == maxExtensions))
		{
			int outId = randomTopImgId(possibleImgs);

			(*locks)[outId].lock();
			(*colorTable)[outId].times_used += 1;
			if (useAltDistanceAlgo)
				(*colorTable)[outId].used_at.push_back((*altCoordsCache)[posXY[1]][posXY[0]]);
			else
				(*colorTable)[outId].used_at.push_back({ posXY[0], posXY[1] });
			(*locks)[outId].unlock();

			return outId;
		}
	}

	std::map<float, int, std::greater<float>> secondChance;
	for (int extDepth = 0; extDepth < closeImgs->size(); extDepth++)
	{
		for (std::pair<float, int> imgPair : (*closeImgs)[extDepth])
		{
			secondChance.insert(std::pair<float, int>(imgPair.first / extDepth, imgPair.second));
		}

	}
	return randomTopImgId(secondChance);
}

int Pixelizer::randomTopImgId(const std::map<float, int, std::greater<float>> &imgs)
{
	if (imgs.size() == 1)
		return imgs.begin()->second;
	std::vector<int> possibleImgs;
	possibleImgs.push_back(imgs.begin()->second);
	float topVal = 0.65 * imgs.begin()->second;

	bool isFirst = true;
	for (auto imgPair : imgs)
	{
		if (isFirst)
		{
			isFirst = false;
			continue;
		}
		if (imgPair.first >= topVal)
			possibleImgs.push_back(imgPair.second);
	}
	
	if (possibleImgs.size() < 2)
		return imgs.begin()->second;

	return possibleImgs[rand() % possibleImgs.size()];
}

std::array<int, 2> Pixelizer::getAlternativeCoodrdinates(int* posXY)
{
	std::array<int, 2> out;
	

	out[0] = posXY[0] / (int)(std::max(templateImg->width(), templateImg->height()) / divisions);
	out[1] = posXY[1] / (int)(std::max(templateImg->width(), templateImg->height()) / divisions);

	return out;
}

std::array<int, 2> Pixelizer::getAlternativeCoodrdinates(std::array<int, 2> posXY)
{
	std::array<int, 2> out;

	out[0] = posXY[0] / (int)(std::max(templateImg->width(), templateImg->height()) / divisions);
	out[1] = posXY[1] / (int)(std::max(templateImg->width(), templateImg->height()) / divisions);

	return out;
}