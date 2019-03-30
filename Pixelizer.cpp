#include "pch.h"
#include "Pixelizer.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <ctime>
#include <cstdlib>

Pixelizer::Pixelizer(const char* targetImgPath, const  char* colorTablePath, const  char* resultFile, float targetImgScalingFactor, int threads, int pixelImgsResolution, float noRepeatRange, int maxExtensions, bool altDistance)
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
	pixelImgPaths = new std::vector<std::vector<std::string>>;

	targetImg = new CImg<unsigned char>(targetImgPath);
	targetImg->resize((int)targetImg->width() * targetImgScalingFactor, (int)targetImg->height() * targetImgScalingFactor, 1, 3, 3);

	if (useAltDistanceAlgo)
	{
		divisions = (int) (std::max(targetImg->width(), targetImg->height()) / noRepeatRange * 1.5);
		altCoordsCache = new std::vector<std::vector<std::array<int, 2>>>(targetImg->height());
		for (int y = 0; y < targetImg->height(); y++)
		{
			(*altCoordsCache)[y].resize(targetImg->width());
			for (int x = 0; x < targetImg->width(); x++)
			{
				(*altCoordsCache)[y][x] = getAlternativeCoodrdinates({ x, y });
			}
		}
	}

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
	colorTable = new std::vector<imgData>;
	nlohmann::json tmpJson = nlohmann::json::parse(fileContent);
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

		colorTable->push_back(tmpData);
	}
	locks = new std::vector<std::mutex>(colorTable->size());
	std::cout << std::endl;
}
 
Pixelizer::~Pixelizer()
{
	if (useAltDistanceAlgo)
		delete altCoordsCache;
	delete colorTable;
	delete targetImg;
	delete pixelImgPaths;
}

void Pixelizer::findImageMatches()
{
	srand(time(NULL));
	pixelImgPaths->resize(targetImg->height());
	for (int y = 0; y < targetImg->height(); y++)
		(*pixelImgPaths)[y].resize(targetImg->width());
	
	matchingProgressPercent = 0;
	threadsDone = 0;

	std::vector<std::thread> ts(threads);
	int threadWidths = targetImg->width() / threads;

	std::thread progressT(&Pixelizer::matchingProgressThread, this);
	for (int i = 0; i < threads - 1; i++)
	{
		ts[i] = std::thread(&Pixelizer::findImageMatchesThreadded, this, threadWidths * i, threadWidths * (i + 1));
	}
	ts[threads - 1] = std::thread(&Pixelizer::findImageMatchesThreadded, this, threadWidths * (threads - 1), targetImg->width());
	for (int i = 0; i < threads; i++)
	{
		ts[i].join();
	}
	progressT.join();
	/*int prevProgress = -1;
	bool prevWasHalf = false;
	std::cout << " [__________________________________________________]";
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

	for (int y = 0; y < targetImg->height(); y++)
	{
		(*pixelImgPaths)[y].resize(targetImg->width());
		for (int x = 0; x < targetImg->width(); x++)
		{
			int posXY[2] = { x, y };
			int tmpRGB[3] = { (*targetImg)(x, y, 0, 0), (*targetImg)(x, y, 0, 1), (*targetImg)(x, y, 0, 2) };
			(*pixelImgPaths)[y][x] = (*colorTable)[findImageMatch(rgbToHsv(tmpRGB), posXY)].file_path;
			if ((int)((100 * (y * targetImg->width() + x)) / (targetImg->height() * targetImg->width())) > prevProgress)
			{
				std::cout << (prevWasHalf ? "=" : "-\b");
				prevWasHalf = !prevWasHalf;
				prevProgress++;
			}
		}
	}*/
	std::cout << std::endl;
}

void Pixelizer::findImageMatchesThreadded(int startX, int endX)
{
	float prevProgress = 0;
	float total = 0;
	int totalPixels = targetImg->height() * targetImg->width();
	for (int y = 0; y < targetImg->height(); y++)
	{
		if (y % 2 == 0)
		{
			float newProgress = (100.f * y * (endX - startX)) / totalPixels;
			progressLock.lock();
			matchingProgressPercent += newProgress - prevProgress;
			progressLock.unlock();
			total += newProgress - prevProgress;
			prevProgress = newProgress;
		}
		for (int x = startX; x < endX; x++)
		{
			int posXY[2] = { x, y };
			int tmpRGB[3] = { (*targetImg)(x, y, 0, 0), (*targetImg)(x, y, 0, 1), (*targetImg)(x, y, 0, 2) };
			(*pixelImgPaths)[y][x] = (*colorTable)[findImageMatch(rgbToHsv(tmpRGB), posXY)].file_path;
		}
		
	}
	float newProgress = (100.f * targetImg->height() * (endX - startX)) / totalPixels;
	progressLock.lock();
	matchingProgressPercent += newProgress - prevProgress;
	progressLock.unlock();

	tsDoneLock.lock();
	threadsDone++;
	tsDoneLock.unlock();
}

void Pixelizer::matchingProgressThread()
{
	float prevProgress = 0;
	std::cout << " [__________________________________________________]";
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

	while (threadsDone != threads)
	{
		Sleep(250);
		if ((int) matchingProgressPercent > (int) prevProgress) {
			progressLock.lock();
			float progDiff = (int) matchingProgressPercent - (int) prevProgress;
			for (int i = 0; i < progDiff; i++)
				std::cout << ((int) (matchingProgressPercent - (progDiff - 1 - i)) % 2 == 0 ? "=" : "-\b");
			prevProgress = matchingProgressPercent;
			progressLock.unlock();
		}
	}
}

CImg<unsigned char> Pixelizer::createFinalImg()
{
	CImg<unsigned char> finalImg(pixelImgsResolution * targetImg->width(), pixelImgsResolution * targetImg->height(), 1, 3, 0);

	int prevProgress = -1;
	bool prevWasHalf = false;
	std::cout << " [__________________________________________________]";
	std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

	for (int y = 0; y < targetImg->height(); y++)
	{
		for (int x = 0; x < targetImg->width(); x++)
		{
			CImg<unsigned char> tmpImg((*pixelImgPaths)[y][x].c_str());
			if (tmpImg.width() != tmpImg.height()) {
				if (tmpImg.height() > tmpImg.width())
					tmpImg.crop(0, (tmpImg.height() - tmpImg.width()) / 2, tmpImg.width(), (tmpImg.width() + (tmpImg.height() - tmpImg.width()) / 2));
				else
					tmpImg.crop((tmpImg.width() - tmpImg.height()) / 2, 0, (tmpImg.height() + (tmpImg.width() - tmpImg.height()) / 2), tmpImg.height());
			}
			tmpImg.resize(pixelImgsResolution, pixelImgsResolution, 1, 3, 3);
			finalImg.draw_image((int) x * pixelImgsResolution, (int) y * pixelImgsResolution, 0, 0, tmpImg);

			if ((int)((100 * (y * targetImg->width() + x)) / (targetImg->height() * targetImg->width())) > prevProgress)
			{
				std::cout << (prevWasHalf ? "=" : "-\b");
				prevWasHalf = !prevWasHalf;
				prevProgress++;
			}
		}
	}

	finalImg.save(resultFile);
	finalImg.display();
	return finalImg;
}

int Pixelizer::findImageMatch(std::array<float, 3> hsvPixels, int posXY[2])
{
	std::vector<std::map<float, int, std::greater<float>>>* closeImgs = new std::vector<std::map<float, int, std::greater<float>>>(maxExtensions + 1);

	//grayscale
	if (hsvPixels[1] < 0.035 || hsvPixels[2] < 0.04)
	{
		//find all images withing range
		for (int i = 0; i < colorTable->size(); i++)
		{
			if ((*colorTable)[i].category != 2)
				continue;
			int extensionsNeeded = abs(hsvPixels[2] - (*colorTable)[i].avr_val) / grayscaleRange;
			if (extensionsNeeded <= maxExtensions)
				(*closeImgs)[extensionsNeeded].insert(std::pair<float, int>((*colorTable)[i].avr_val_score, i));
		}

		//if images have been found find the best fitting one
		if (!closeImgs->empty()){

			//evaluate all found images and return the first best image
			for (int extDepth = 0; extDepth < closeImgs->size(); extDepth++)
			{
				std::map<float, int, std::greater<float>> possibleImgs;
				for (std::pair<float, int> valueScore : (*closeImgs)[extDepth])
				{
					if ((*colorTable)[valueScore.second].times_used == 0)
					{
						(*locks)[valueScore.second].lock();
						(*colorTable)[valueScore.second].times_used += 1;
						if (useAltDistanceAlgo)
							(*colorTable)[valueScore.second].used_at.push_back(getAlternativeCoodrdinates(posXY));
						else
							(*colorTable)[valueScore.second].used_at.push_back({ posXY[0], posXY[1] });
						(*locks)[valueScore.second].unlock();
						int tmpOut = valueScore.second;
						delete closeImgs;
						return tmpOut;
					}

					//check for distance to same picture
					bool imgUsable = true;
					//bool imgUsable = (*colorTable)[valueScore.second].times_used < 7;
					for (int i = 0; i < (*colorTable)[valueScore.second].times_used && imgUsable; i++)
					{
						if (useAltDistanceAlgo)
						{
							if (abs((*altCoordsCache)[posXY[1]][posXY[0]][0] - (*colorTable)[valueScore.second].used_at[i][0]) <= 1 &&
								abs((*altCoordsCache)[posXY[1]][posXY[0]][1] - (*colorTable)[valueScore.second].used_at[i][1]) <= 1)
							{
								imgUsable = false;
							}
						}
						else
						{
							if (pow(posXY[0] - (*colorTable)[valueScore.second].used_at[i][0], 2) +
								pow(posXY[1] - (*colorTable)[valueScore.second].used_at[i][1], 2)
								<= noRepeatRange)
							{
								imgUsable = false;
							}
						}
					}

					if (imgUsable)
					{
						possibleImgs.insert(valueScore);
					}
				}

				if (!possibleImgs.empty())
				{
					delete closeImgs;
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
			delete closeImgs;
			return secondChance.begin()->second;
		}
		
	}
	//colorful pixel :)
	else
	{
		bool isInThinHueRange = distanceOnCircle(hsvPixels[0], 60) <= 11 && hsvPixels[0] - 60 <= 8 ||
			distanceOnCircle(hsvPixels[0], 180) <= 25 || 
			distanceOnCircle(hsvPixels[0], 300) <= 30;
		//find all images withing range
		for (int i = 0; i < colorTable->size(); i++)
		{
			if ((*colorTable)[i].category != 0)
				continue;

			float satDiff = abs(hsvPixels[1] - (float) (*colorTable)[i].avr_sat);
			float valDiff = abs(hsvPixels[2] - (float) (*colorTable)[i].avr_val);
			int satExtensionCount = satDiff / saturationRange;
			int valExtensionCount = valDiff / valueRange;
			if (satExtensionCount > maxExtensions || valExtensionCount > maxExtensions)
				continue;

			int hueDiff = distanceOnCircle((*colorTable)[i].max_hue, hsvPixels[0]);
			int hueDiffAbs = abs(hsvPixels[0] - (*colorTable)[i].max_hue);
			
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
						if ((edgeCase - (*colorTable)[i].max_hue) * (edgeCase - hsvPixels[0]) < 0)
							skipImage = true;
					}
					else
					{
						int tmpMinHue = 360 + std::min(hsvPixels[0], (*colorTable)[i].max_hue);
						int tmpMaxHue = std::max(hsvPixels[0], (*colorTable)[i].max_hue);
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

			/*if (distanceOnCircle(hsvPixels[0], (*colorTable)[i].max_hue) < hueRange * recursionCount &&
				abs(hsvPixels[1] - (float) (*colorTable)[i].avr_sat) < saturationRange * recursionCount&&
				abs(hsvPixels[2] - (float) (*colorTable)[i].avr_val) < valueRange * recursionCount)
				closeImgs->insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));*/

			/*if (abs(hsvPixels[1] - (float) (*colorTable)[i].avr_sat) > saturationRange * recursionCount ||
				abs(hsvPixels[2] - (float) (*colorTable)[i].avr_val) > valueRange * recursionCount)
				continue;*/

			/*if (isInThinHueRange)
			{
				if (distanceOnCircle(hsvPixels[0], (*colorTable)[i].max_hue) < hueRange * recursionCount * 0.6)
					closeImgs->insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));
				else
					continue;
			}
			else if (leftLimit >= 0 && rightLimit < 360)
			{
				if ((*colorTable)[i].max_hue > leftLimit && (*colorTable)[i].max_hue < rightLimit)
					closeImgs->insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));
			}
			else if (leftLimit < 0 && rightLimit < 360)
			{
				if (((*colorTable)[i].max_hue > (360 + leftLimit) || (*colorTable)[i].max_hue >= 0) && (*colorTable)[i].max_hue < rightLimit)
					closeImgs->insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));
			}
			else if (leftLimit >= 0 && rightLimit >= 360)
			{
				if ((*colorTable)[i].max_hue > leftLimit && ((*colorTable)[i].max_hue < (rightLimit - 360) || (*colorTable)[i].max_hue < 360))
					closeImgs->insert(std::pair<float, int>((*colorTable)[i].max_hue_score, i));
			}*/

		}

		//find image where the target hue appears the most often
		if (!closeImgs->empty())
		{
			//evaluate all found images and return the first best image
			for (int extDepth = 0; extDepth < closeImgs->size(); extDepth++)
			{
				std::map<float, int, std::greater<float>> possibleImgs;
				for (std::pair<float, int> hueScore : (*closeImgs)[extDepth])
				{
					if ((*colorTable)[hueScore.second].times_used == 0)
					{
						(*locks)[hueScore.second].lock();
						(*colorTable)[hueScore.second].times_used += 1;
						if (useAltDistanceAlgo)
							(*colorTable)[hueScore.second].used_at.push_back(getAlternativeCoodrdinates(posXY));
						else
							(*colorTable)[hueScore.second].used_at.push_back({ posXY[0], posXY[1] });
						(*locks)[hueScore.second].unlock();
						int tmpOut = hueScore.second;
						delete closeImgs;
						return tmpOut;
					}

					//check for distance to same picture, that are already placed
					bool imgUsable = true;
					//bool imgUsable = (*colorTable)[hueScore.second].times_used < 7;
					for (int i = 0; i < (*colorTable)[hueScore.second].times_used && imgUsable; i++)
					{
						if (useAltDistanceAlgo)
						{
							if (abs((*altCoordsCache)[posXY[1]][posXY[0]][0] - (*colorTable)[hueScore.second].used_at[i][0]) <= 1 &&
								abs((*altCoordsCache)[posXY[1]][posXY[0]][1] - (*colorTable)[hueScore.second].used_at[i][1]) <= 1)
							{
								imgUsable = false;
							}
						}
						else
						{
							if (pow(posXY[0] - (*colorTable)[hueScore.second].used_at[i][0], 2) +
								pow(posXY[1] - (*colorTable)[hueScore.second].used_at[i][1], 2)
								<= noRepeatRange)
							{
								imgUsable = false;
							}
						}

					}
					if (imgUsable)
					{
						possibleImgs.insert(hueScore);
					}
				}
				if (!possibleImgs.empty())
				{
					delete closeImgs;
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
			delete closeImgs;
			return secondChance.begin()->second;
		}
	}



	delete closeImgs;
	return 0;
}

int Pixelizer::randomTopImgId(const std::map<float, int, std::greater<float>> &imgs)
{
	if (imgs.size() == 1)
		return imgs.begin()->second;
	std::vector<int> possibleImgs;
	possibleImgs.push_back(imgs.begin()->second);
	float topVal = 0.75 * imgs.begin()->second;

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
	

	out[0] = posXY[0] / (int)(std::max(targetImg->width(), targetImg->height()) / divisions);
	out[1] = posXY[1] / (int)(std::max(targetImg->width(), targetImg->height()) / divisions);

	return out;
}

std::array<int, 2> Pixelizer::getAlternativeCoodrdinates(std::array<int, 2> posXY)
{
	std::array<int, 2> out;

	out[0] = posXY[0] / (int)(std::max(targetImg->width(), targetImg->height()) / divisions);
	out[1] = posXY[1] / (int)(std::max(targetImg->width(), targetImg->height()) / divisions);

	return out;
}

float Pixelizer::distanceOnCircle(float val1, float val2)
{
	int minDistance = abs(val1 - val2);

	return minDistance > 180 ? 360 - minDistance : minDistance;
}

