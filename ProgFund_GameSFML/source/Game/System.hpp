#pragma once
#include "../../pch.h"

#include "GConstant.hpp"

class SystemUtility {
private:
	SystemUtility() {};	//Static class
public:
	static DxRect<int> GetAsciiRect(char ch, int rect_w = 16, int rect_h = 16) {
		int rect_i[] = { 0, 32 };

#define DEF_RECT(chr, row, index) case chr: rect_i[0] = row * 32 + index; break;
		switch (ch) {
			//Row 1
			DEF_RECT('\0', 0, 0);
			DEF_RECT('!', 0, 1);
			DEF_RECT('\"', 0, 2);
			DEF_RECT('#', 0, 3);
			DEF_RECT('$', 0, 4);
			DEF_RECT('%', 0, 5);
			DEF_RECT('&', 0, 6);
			DEF_RECT('\'', 0, 7);
			DEF_RECT('(', 0, 8);
			DEF_RECT(')', 0, 9);
			DEF_RECT('*', 0, 10);
			DEF_RECT('+', 0, 11);
			DEF_RECT(',', 0, 12);
			DEF_RECT('-', 0, 13);
			DEF_RECT('.', 0, 14);
			DEF_RECT('/', 0, 15);
			DEF_RECT('0', 0, 16);
			DEF_RECT('1', 0, 17);

			//Row 2
			DEF_RECT('2', 1, 0);
			DEF_RECT('3', 1, 1);
			DEF_RECT('4', 1, 2);
			DEF_RECT('5', 1, 3);
			DEF_RECT('6', 1, 4);
			DEF_RECT('7', 1, 5);
			DEF_RECT('8', 1, 6);
			DEF_RECT('9', 1, 7);
			DEF_RECT(':', 1, 8);
			DEF_RECT(';', 1, 9);
			DEF_RECT('<', 1, 10);
			DEF_RECT('=', 1, 11);
			DEF_RECT('>', 1, 12);
			DEF_RECT('?', 1, 13);
			DEF_RECT('@', 1, 14);
			DEF_RECT('A', 1, 15);
			DEF_RECT('B', 1, 16);
			DEF_RECT('C', 1, 17);

			//Row 3
			DEF_RECT('D', 2, 0);
			DEF_RECT('E', 2, 1);
			DEF_RECT('F', 2, 2);
			DEF_RECT('G', 2, 3);
			DEF_RECT('H', 2, 4);
			DEF_RECT('I', 2, 5);
			DEF_RECT('J', 2, 6);
			DEF_RECT('K', 2, 7);
			DEF_RECT('L', 2, 8);
			DEF_RECT('M', 2, 9);
			DEF_RECT('N', 2, 10);
			DEF_RECT('O', 2, 11);
			DEF_RECT('P', 2, 12);
			DEF_RECT('Q', 2, 13);
			DEF_RECT('R', 2, 14);
			DEF_RECT('S', 2, 15);
			DEF_RECT('T', 2, 16);
			DEF_RECT('U', 2, 17);

			//Row 4
			DEF_RECT('V', 3, 0);
			DEF_RECT('W', 3, 1);
			DEF_RECT('X', 3, 2);
			DEF_RECT('Y', 3, 3);
			DEF_RECT('Z', 3, 4);
			DEF_RECT('[', 3, 5);
			DEF_RECT('\\', 3, 6);
			DEF_RECT(']', 3, 7);
			DEF_RECT('^', 3, 8);
			DEF_RECT('_', 3, 9);
			DEF_RECT(' ', 3, 10);
			DEF_RECT('a', 3, 11);
			DEF_RECT('b', 3, 12);
			DEF_RECT('c', 3, 13);
			DEF_RECT('d', 3, 14);
			DEF_RECT('e', 3, 15);
			DEF_RECT('f', 3, 16);
			DEF_RECT('g', 3, 17);

			//Row 5
			DEF_RECT('h', 4, 0);
			DEF_RECT('i', 4, 1);
			DEF_RECT('j', 4, 2);
			DEF_RECT('k', 4, 3);
			DEF_RECT('l', 4, 4);
			DEF_RECT('m', 4, 5);
			DEF_RECT('n', 4, 6);
			DEF_RECT('o', 4, 7);
			DEF_RECT('p', 4, 8);
			DEF_RECT('q', 4, 9);
			DEF_RECT('r', 4, 10);
			DEF_RECT('s', 4, 11);
			DEF_RECT('t', 4, 12);
			DEF_RECT('u', 4, 13);
			DEF_RECT('v', 4, 14);
			DEF_RECT('w', 4, 15);
			DEF_RECT('x', 4, 16);
			DEF_RECT('y', 4, 17);

			//Row 6
			DEF_RECT('z', 5, 0);
			DEF_RECT('{', 5, 1);
			DEF_RECT('|', 5, 2);
			DEF_RECT('}', 5, 3);
			DEF_RECT('~', 5, 4);
			DEF_RECT('\xf0', 5, 5);	//BS
			DEF_RECT('\xf1', 5, 6);	//END
			DEF_RECT('\xf2', 5, 7);	//Square
		}
#undef DEF_RECT

		return DxRect<int>::SetFromIndex(rect_w, rect_h, rect_i[0], rect_i[1]);
	}
};

class UtilTask_FadeBGM : public TaskBase {
private:
	shared_ptr<SoundResource> music_;
	double baseVol_;
public:
	UtilTask_FadeBGM(Scene* parent, shared_ptr<SoundResource> music, size_t frameFade, double baseVol) : TaskBase(parent) {
		music_ = music;
		frameEnd_ = frameFade;
		baseVol_ = baseVol;
	}
	~UtilTask_FadeBGM() {
		music_->GetData()->Stop();
	}

	virtual void Render() {};
	virtual void Update() {
		double vol = Math::Lerp::Linear(baseVol_, 0.0, frame_ / (double)(frameEnd_ - 1));
		music_->GetData()->SetVolumeRate(vol);
		++frame_;
	}
};
class UtilTask_ColorFade : public TaskBase {
private:
	Sprite2D objFade_;
	size_t framePos_[3];
public:
	UtilTask_ColorFade(Scene* parent,  
		size_t frameIn, size_t frameStay, size_t frameOut, D3DCOLOR color) : TaskBase(parent) 
	{
		framePos_[0] = frameIn;
		framePos_[1] = frameStay;
		framePos_[2] = frameOut;
		frameEnd_ = frameIn + frameStay + frameOut + 1;
		{
			objFade_.SetSourceRect(DxRect(0, 0, 640, 480));
			objFade_.SetDestRect(DxRect(0, 0, 640, 480));
			objFade_.SetColor(color);
			objFade_.UpdateVertexBuffer();
		}
	}

	virtual void Render() {
		objFade_.Render();
	}
	virtual void Update() {
		if (frame_ < framePos_[0]) {
			objFade_.SetAlpha(Math::Lerp::Smooth(0, 255, frame_ / (float)framePos_[0]));
		}
		else if (frame_ >= framePos_[0] + framePos_[1]) {
			size_t framePeriod = frame_ - (framePos_[0] + framePos_[1]);
			objFade_.SetAlpha(Math::Lerp::Smooth(255, 0, framePeriod / (float)framePos_[2]));
		}
		else
			objFade_.SetAlpha(255);

		++frame_;
	}
};