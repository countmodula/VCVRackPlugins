//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - LED Displays
//  Copyright (C) 2020  Adam Verspaget 
//----------------------------------------------------------------------------
#include "componentlibrary.hpp"

using namespace rack;


//-------------------------------------------------------------------
// LED Display Base
//-------------------------------------------------------------------
struct CountModulaLEDDisplay : ModuleLightWidget {
	std::shared_ptr<Font> font;
	std::string text;
	float fontSize;
	Vec textPos;
	int numChars = 2;
	
	int colorScheme = 0;
	NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
	NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);	
	NVGcolor textColor = nvgRGB(0xff, 0x10, 0x10);		
			
	enum LEDDisplayColorScheme {
		RedLEDDisplay,
		GreenLEDDisplay,
		YellowLEDDisplay,
		BlueLEDDisplay
	};			
			
	void setCentredPos(Vec pos) {
		box.pos.x = pos.x - box.size.x/2;
		box.pos.y = pos.y - box.size.y/2;
	}
	
	void setLeftPos(Vec pos) {
		box.pos.x = pos.x;
		box.pos.y = pos.y - box.size.y/2;
	}
	
	void setRightPos(Vec pos) {
		box.pos.x = pos.x - box.size.x;
		box.pos.y = pos.y - box.size.y/2;
	}
	
	void setColorScheme(int schemeId) {
		switch (schemeId) {
			case GreenLEDDisplay:
				backgroundColor = nvgRGB(0x14, 0x24, 0x14);
				borderColor = nvgRGB(0x10, 0x10, 0x10);	
				textColor = nvgRGB(0x10, 0xff, 0x10);				
				break;
			case YellowLEDDisplay:
				backgroundColor = nvgRGB(0x24, 0x24, 0x14);
				borderColor = nvgRGB(0x10, 0x10, 0x10);	
				textColor = nvgRGB(0xff, 0xff, 0x10);				
				break;
			case BlueLEDDisplay:
				backgroundColor = nvgRGB(0x14, 0x14, 0x24);
				borderColor = nvgRGB(0x10, 0x10, 0x10);	
				textColor = nvgRGB(0x09, 0x86, 0xdd);
				break;
			default:
				backgroundColor = nvgRGB(0x24, 0x14, 0x14);
				borderColor = nvgRGB(0x10, 0x10, 0x10);	
				textColor = nvgRGB(0xff, 0x10, 0x10);				
				break;
		}
	}
		
	void drawBackground(const DrawArgs &args) override {
		// Background
		// NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		// NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);
	}
	
	void drawLight(const DrawArgs &args) override {
		char buffer[numChars+1];
		int l = text.size();
		if (l > numChars)
			l = numChars;
			
		nvgGlobalTint(args.vg, color::WHITE);
		
		text.copy(buffer, l);
		buffer[numChars] = '\0';
		
		// Background
		// NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		// NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		if (font  && font->handle >= 0) {
			nvgFontSize(args.vg, fontSize);
			nvgFontFaceId(args.vg, font->handle);
			nvgTextLetterSpacing(args.vg, 1);

			// NVGcolor textColor = nvgRGB(0xff, 0x10, 0x10);
			
			// render the "off" segments 	
			nvgFillColor(args.vg, nvgTransRGBA(textColor, 18));
			nvgText(args.vg, textPos.x, textPos.y, "~~", NULL);
			
			// render the "on segments"
			nvgFillColor(args.vg, textColor);
			nvgText(args.vg, textPos.x, textPos.y, buffer, NULL);
		}
	}
};


struct CountModulaLEDDisplayMega : CountModulaLEDDisplay {
	std::string format;
	
	CountModulaLEDDisplayMega(int digits) {
		numChars = digits;
		fontSize = 48;
		box.size = Vec(digits * 44, 60);
		textPos = Vec(7, 54);
		format = rack::string::f("%c%02dd", '%', digits);
	}
	
	void setText(int n) {
		text = rack::string::f(format.c_str(), n);
	}
};


struct CountModulaLEDDisplayLarge : CountModulaLEDDisplay {
	std::string format;
	
	CountModulaLEDDisplayLarge(int digits) {
		numChars = digits;
		fontSize = 28;
		box.size = Vec(digits * 25, 40);
		textPos = Vec(3, 34);
		format = rack::string::f("%c%02dd", '%', digits);
		setColorScheme(0);
	}
	
	void setText(int n) {
		text = rack::string::f(format.c_str(), n);
	}
};

struct CountModulaLEDDisplayMedium : CountModulaLEDDisplay {
	std::string format;
	
	CountModulaLEDDisplayMedium(int digits) {
		numChars = digits;
		fontSize = 20;
		box.size = Vec(digits * 19.4, 26);
		textPos = Vec(4, 23);
		format = rack::string::f("%c%02dd", '%', digits);
		setColorScheme(0);
	}
	
	void setText(int n) {
		text = rack::string::f(format.c_str(), n);
	}
};

struct CountModulaLEDDisplaySmall : CountModulaLEDDisplay {
	std::string format;
	
	CountModulaLEDDisplaySmall(int digits) {
		numChars = digits;
		fontSize = 14;
		box.size = Vec(digits * 12.5, 20);
		textPos = Vec(1, 17);
		format = rack::string::f("%c%02dd", '%', digits);
		setColorScheme(0);
	}
	
	void setText(int n) {
		text = rack::string::f(format.c_str(), n);
	}
};

struct CountModulaLEDDisplayMini2 : CountModulaLEDDisplay {
	CountModulaLEDDisplayMini2() {
		numChars = 2;
		fontSize = 14;
		box.size = Vec(25, 20);
		textPos = Vec(1, 17);
		setColorScheme(0);
	}
};

