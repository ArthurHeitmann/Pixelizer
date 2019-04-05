#pragma once
#include <string>
#include <vector>
#include <array>

struct imgData {
	float avr_sat, avr_val, avr_val_score, max_hue, max_hue_score;
	int category, times_used = 0;
	std::string file_path;
	std::vector<std::array<int, 2>> used_at;
};

class PixTools 
{

public:
	/*
	 Returns minimal distance in degrees on a circle, from 2 given angles
	*/
	static int distanceOnCircle(float val1, float val2)
	{
		int minDistance = abs(val1 - val2);
		return minDistance > 180 ? 360 - minDistance : minDistance;
	}

	/* takes a hsv array ([0]: 0 - 360; [1]: 0.0 - 1.0; [2]: 0.0 - 1.0) and retuns the converted rgb values in an array */
	static int* hsvToRgb(float hsv[3])
	{
		float tmpRgb[3];
		int out[3];
		int i;
		float f, p, q, t;

		if (hsv[1] < 0.01)
		{
			out[0] = (int) (hsv[2] * 255);
			out[1] = (int) (hsv[2] * 255);
			out[2] = (int) (hsv[2] * 255);
			//out = { (int)(hsv[2] * 255), (int)(hsv[2] * 255), (int)(hsv[2] * 255) };
			return out;
		}
		hsv[0] /= 60;
		i = hsv[0];
		f = hsv[0] - i;
		p = hsv[2] * (1 - hsv[1]);
		q = hsv[2] * (1 - hsv[1] * f);
		t = hsv[2] * (1 - hsv[1] * (1 - f));

		switch (i)
		{
		case 0:
			tmpRgb[0] = hsv[2];
			tmpRgb[1] = t;
			tmpRgb[2] = p;
			break;
		case 1:
			tmpRgb[0] = q;
			tmpRgb[1] = hsv[2];
			tmpRgb[2] = p;
			break;
		case 2:
			tmpRgb[0] = p;
			tmpRgb[1] = hsv[2];
			tmpRgb[2] = t;
			break;
		case 3:
			tmpRgb[0] = p;
			tmpRgb[1] = q;
			tmpRgb[2] = hsv[2];
			break;
		case 4:
			tmpRgb[0] = t;
			tmpRgb[1] = p;
			tmpRgb[2] = hsv[2];
			break;
		default:
			tmpRgb[0] = hsv[2];
			tmpRgb[1] = p;
			tmpRgb[2] = q;
			break;
		}

		tmpRgb[0] *= 255;
		tmpRgb[1] *= 255;
		tmpRgb[2] *= 255;

		out[0] = (int) round(tmpRgb[0]);
		out[1] = (int) round(tmpRgb[1]);
		out[2] = (int) round(tmpRgb[2]);
		//out = { (int)round(tmpRgb[0]), (int)round(tmpRgb[1]),(int)round(tmpRgb[2]) };

		return out;
	}

	/* takes a rgb array ([0]: 0 - 255; [1]: 0 - 255; [2]: 0 - 255) and retuns the converted hsv values ([0]: 0 - 360; [1]: 0.0 - 1.0; [2]: 0.0 - 1.0) in an array */
	static std::array<float, 3> rgbToHsv(int rgb[3])
	{
		//straight from so
		std::array<float, 3> hsv;
		int min = rgb[0] < rgb[1] ? rgb[0] : rgb[1];
		min = min < rgb[2] ? min : rgb[2];
		int max = rgb[0] > rgb[1] ? rgb[0] : rgb[1];
		max = max > rgb[2] ? max : rgb[2];

		hsv[2] = (float) max / 255;
		int delta = max - min;
		if (delta == 0)
		{
			hsv[0] = 0;
			hsv[1] = 0;
			return hsv;
		}

		if (max > 0)
		{
			hsv[1] = (float) delta / max;
		}
		else
		{
			hsv[0] = 0;
			hsv[1] = 0;
			return hsv;
		}

		if (rgb[0] == max)
			hsv[0] = (float) (rgb[1] - rgb[2]) / delta;
		else if (rgb[1] == max)
			hsv[0] = 2 + (float) (rgb[2] - rgb[0]) / delta;
		else
			hsv[0] = 4 + (float) (rgb[0] - rgb[1]) / delta;

		hsv[0] *= 60;
		if (hsv[0] < 0)
			hsv[0] += 360;

		return hsv;
	}

};