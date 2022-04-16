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
	
	void setCentredPos(Vec pos) {
		box.pos.x = pos.x - box.size.x/2;
		box.pos.y = pos.y - box.size.y/2;
	}
	
	void drawBackground(const DrawArgs &args) override {
		// Background
		NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
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
		NVGcolor backgroundColor = nvgRGB(0x24, 0x14, 0x14);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
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

			NVGcolor textColor = nvgRGB(0xff, 0x10, 0x10);
			
			// render the "off" segments 	
			nvgFillColor(args.vg, nvgTransRGBA(textColor, 18));
			nvgText(args.vg, textPos.x, textPos.y, "~~", NULL);
			
			// render the "on segments"
			nvgFillColor(args.vg, textColor);
			nvgText(args.vg, textPos.x, textPos.y, buffer, NULL);
		}
	}
};

struct CountModulaLEDDisplayLarge2 : CountModulaLEDDisplay {
	CountModulaLEDDisplayLarge2() {
		numChars = 2;
		fontSize = 28;
		box.size = Vec(50, 40);
		textPos = Vec(3, 34);
	}
};

struct CountModulaLEDDisplayLarge3 : CountModulaLEDDisplay {
	CountModulaLEDDisplayLarge3() {
		numChars = 3;
		fontSize = 28;
		box.size = Vec(75, 40);
		textPos = Vec(3, 34);
	}
};

struct CountModulaLEDDisplayMini2 : CountModulaLEDDisplay {
	CountModulaLEDDisplayMini2() {
		numChars = 2;
		fontSize = 14;
		box.size = Vec(25, 20);
		textPos = Vec(1, 17);
	}
};

