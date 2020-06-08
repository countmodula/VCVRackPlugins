//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Custom components
//  Copyright (C) 2019  Adam Verspaget 
//----------------------------------------------------------------------------
#include "componentlibrary.hpp"

using namespace rack;


//-------------------------------------------------------------------
// screws
//-------------------------------------------------------------------
struct CountModulaScrew : SVGScrew {
	CountModulaScrew() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/ScrewHex.svg")));
		box.size = sw->box.size;
	}
};


//-------------------------------------------------------------------
// Lights
//-------------------------------------------------------------------
struct CountModulaLightRYG : GrayModuleLightWidget {
	CountModulaLightRYG() {
		addBaseColor(SCHEME_RED);
		addBaseColor(SCHEME_YELLOW);
		addBaseColor(SCHEME_GREEN);
	}
};

struct CountModulaLightRG : GrayModuleLightWidget {
	CountModulaLightRG() {
		addBaseColor(SCHEME_RED);
		addBaseColor(SCHEME_GREEN);
	}
};

struct CountModulaLightWB : GrayModuleLightWidget {
	CountModulaLightWB() {
		addBaseColor(SCHEME_WHITE);
		addBaseColor(SCHEME_BLUE);
	}
};

//-------------------------------------------------------------------
// Ports
//-------------------------------------------------------------------
struct CountModulaJack : SVGPort {
	CountModulaJack() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Jack.svg")));
	}
};

//-------------------------------------------------------------------
// Knobs
//-------------------------------------------------------------------
// TODO: parameterise the colour

// base knob
struct CountModulaKnob : SvgKnob {
	CountModulaKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};

// coloured knobs
struct CountModulaKnobRed : CountModulaKnob {
	CountModulaKnobRed() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobRed.svg")));
	}
};

struct CountModulaKnobOrange : CountModulaKnob {
	CountModulaKnobOrange() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobOrange.svg")));
	}
};

struct CountModulaKnobYellow : CountModulaKnob {
	CountModulaKnobYellow() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobYellow.svg")));
	}
};

struct CountModulaKnobGreen : CountModulaKnob {
	CountModulaKnobGreen() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobGreen.svg")));
	}
};

struct CountModulaKnobBlue : CountModulaKnob {
	CountModulaKnobBlue() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobBlue.svg")));
	}
};

struct CountModulaKnobViolet : CountModulaKnob {
	CountModulaKnobViolet() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobViolet.svg")));
	}
};

struct CountModulaKnobGrey : CountModulaKnob {
	CountModulaKnobGrey() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobGrey.svg")));
	}
};

struct CountModulaKnobWhite : CountModulaKnob {
	CountModulaKnobWhite() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobWhite.svg")));
	}
};

// coloured knobs - mega
struct CountModulaKnobMegaRed : CountModulaKnob {
	CountModulaKnobMegaRed() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaRed.svg")));
	}
};

struct CountModulaKnobMegaOrange : CountModulaKnob {
	CountModulaKnobMegaOrange() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaOrange.svg")));
	}
};

struct CountModulaKnobMegaYellow : CountModulaKnob {
	CountModulaKnobMegaYellow() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaYellow.svg")));
	}
};

struct CountModulaKnobMegaGreen : CountModulaKnob {
	CountModulaKnobMegaGreen() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaGreen.svg")));
	}
};

struct CountModulaKnobMegaBlue : CountModulaKnob {
	CountModulaKnobMegaBlue() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaBlue.svg")));
	}
};

struct CountModulaKnobMegaViolet : CountModulaKnob {
	CountModulaKnobMegaViolet() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaViolet.svg")));
	}
};

struct CountModulaKnobMegaGrey : CountModulaKnob {
	CountModulaKnobMegaGrey() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaGrey.svg")));
	}
};

struct CountModulaKnobMegaWhite : CountModulaKnob {
	CountModulaKnobMegaWhite() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMegaWhite.svg")));
	}
};

//-------------------------------------------------------------------
// rotary switches
//-------------------------------------------------------------------
// TODO: parameterise the colour

struct CountModulaRotarySwitch : SvgKnob {
	CountModulaRotarySwitch() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		snap = true;
		smooth = false;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgKnob::onChange(e);
		
		paramQuantity->setValue(roundf(paramQuantity->getValue()));
	}
	
	
	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgKnob::randomize();
		
		paramQuantity->setValue(roundf(paramQuantity->getValue()));
	}	
	
};

struct CountModulaRotarySwitchRed : CountModulaRotarySwitch {
	CountModulaRotarySwitchRed() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobRed.svg")));
	}
};
struct CountModulaRotarySwitchOrange : CountModulaRotarySwitch {
	CountModulaRotarySwitchOrange() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobOrange.svg")));
	}
};

struct CountModulaRotarySwitchYellow : CountModulaRotarySwitch {
	CountModulaRotarySwitchYellow() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobYellow.svg")));
	}
};

struct CountModulaRotarySwitchGreen : CountModulaRotarySwitch {
	CountModulaRotarySwitchGreen() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobGreen.svg")));
	}
};
struct CountModulaRotarySwitchBlue : CountModulaRotarySwitch {
	CountModulaRotarySwitchBlue() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobBlue.svg")));
	}
};
struct CountModulaRotarySwitchGrey : CountModulaRotarySwitch {
	CountModulaRotarySwitchGrey() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobGrey.svg")));
	}
};
struct CountModulaRotarySwitchViolet : CountModulaRotarySwitch {
	CountModulaRotarySwitchViolet() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobViolet.svg")));
	}
};
struct CountModulaRotarySwitchWhite : CountModulaRotarySwitch {
	CountModulaRotarySwitchWhite() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobWhite.svg")));
	}
};

//-------------------------------------------------------------------
// 5 pos rotary switches
//-------------------------------------------------------------------
// TODO: parameterise the number of positions and the colour

struct CountModulaRotarySwitch5PosRed : CountModulaRotarySwitchRed {
	CountModulaRotarySwitch5PosRed() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosOrange : CountModulaRotarySwitchOrange {
	CountModulaRotarySwitch5PosOrange() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosYellow : CountModulaRotarySwitchYellow {
	CountModulaRotarySwitch5PosYellow() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosGreen : CountModulaRotarySwitchGreen {
	CountModulaRotarySwitch5PosGreen() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosBlue : CountModulaRotarySwitchBlue {
	CountModulaRotarySwitch5PosBlue() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosGrey : CountModulaRotarySwitchGrey {
	CountModulaRotarySwitch5PosGrey() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosViolet : CountModulaRotarySwitchViolet {
	CountModulaRotarySwitch5PosViolet() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosWhite : CountModulaRotarySwitchWhite {
	CountModulaRotarySwitch5PosWhite() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

//-------------------------------------------------------------------
// on-off toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle2P : SvgSwitch {
	int pos;
	int neg;

	CountModulaToggle2P() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_2.svg")));

		// no shadow for switches
		shadow->opacity = 0.0f;

		neg = pos = 0;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		if (paramQuantity->getValue() > 0.5f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}
	
#ifdef NEW_TOGGLE_CHANGE_METHOD	
	void onDoubleClick(const event::DoubleClick& e) override {
		reset();
	}		
	
	void onDragStart(const event::DragStart& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		
		pos = 0;
		neg=0;
	}
	
	void onDragEnd(const event::DragEnd& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
	}
	
	void onDragMove(const event::DragMove& e) override {
		
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		if (e.mouseDelta.y < 0.0f) {
			neg = 0;
			pos++;
		}
		else if (e.mouseDelta.y > 0.0f) {
			pos = 0;
			neg++;
		}
		
		if (neg > 5) {
			paramQuantity->setValue(paramQuantity->getValue() - 1.0f);
			neg = 0;
		}
		else if (pos > 5) {
			paramQuantity->setValue(paramQuantity->getValue() + 1.0f);
			pos = 0;
		}
	}
#endif
	
	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgSwitch::randomize();

		if (paramQuantity->getValue() > 0.5f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}	
};

//-------------------------------------------------------------------
// on-off toggle switch - sideways
//-------------------------------------------------------------------
struct CountModulaToggle2P90 : SvgSwitch {
	int pos;
	int neg;
	
	CountModulaToggle2P90() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));

		// no shadow for switches
		shadow->opacity = 0.0f;

		neg = pos = 0;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		if (paramQuantity->getValue() > 0.5f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}

#ifdef NEW_TOGGLE_CHANGE_METHOD	
	void onDoubleClick(const event::DoubleClick& e) override {
		reset();
	}	
	
	void onDragStart(const event::DragStart& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		
		pos = 0;
		neg=0;
	}
	
	void onDragEnd(const event::DragEnd& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
	}
	
	void onDragMove(const event::DragMove& e) override {
		
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		if (e.mouseDelta.x < 0.0f) {
			neg = 0;
			pos++;
		}
		else if (e.mouseDelta.x > 0.0f) {
			pos = 0;
			neg++;
		}
		
		if (neg > 5) {
			paramQuantity->setValue(paramQuantity->getValue() - 1.0f);
			neg = 0;
		}
		else if (pos > 5) {
			paramQuantity->setValue(paramQuantity->getValue() + 1.0f);
			pos = 0;
		}
	}	
#endif
	
	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgSwitch::randomize();

		if (paramQuantity->getValue() > 0.5f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}	
};

//-------------------------------------------------------------------
// on-off-on toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle3P : SvgSwitch {
	int pos;
	int neg;
	
	CountModulaToggle3P() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_2.svg")));
		
		// no shadow for switches
		shadow->opacity = 0.0f;
		
		neg = pos = 0;
	}

	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		if (paramQuantity->getValue() > 1.33f)
			paramQuantity->setValue(2.0f);
		else if (paramQuantity->getValue() > 0.67f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}

#ifdef NEW_TOGGLE_CHANGE_METHOD	
	void onDoubleClick(const event::DoubleClick& e) override {
		reset();
	}	
	
	void onDragStart(const event::DragStart& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		
		pos = 0;
		neg=0;
	}
	
	void onDragEnd(const event::DragEnd& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
	}
	
	void onDragMove(const event::DragMove& e) override {
		
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		if (e.mouseDelta.y < 0.0f) {
			neg = 0;
			pos++;
		}
		else if (e.mouseDelta.y > 0.0f) {
			pos = 0;
			neg++;
		}
		
		if (neg > 5) {
			paramQuantity->setValue(paramQuantity->getValue() - 1.0f);
			neg = 0;
		}
		else if (pos > 5) {
			paramQuantity->setValue(paramQuantity->getValue() + 1.0f);
			pos = 0;
		}
	}
#endif
	
	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgSwitch::randomize();
		
		if (paramQuantity->getValue() > 1.33f)
			paramQuantity->setValue(2.0f);
		else if (paramQuantity->getValue() > 0.67f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// on-off-on toggle switch - sideways
//-------------------------------------------------------------------
struct CountModulaToggle3P90 : SvgSwitch {
	int pos;
	int neg;
	
	CountModulaToggle3P90() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));
		
		// no shadow for switches
		shadow->opacity = 0.0f;
		
		neg = pos = 0;
	}

	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		if (paramQuantity->getValue() > 1.33f)
			paramQuantity->setValue(2.0f);
		else if (paramQuantity->getValue() > 0.67f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}

#ifdef NEW_TOGGLE_CHANGE_METHOD	
	void onDragStart(const event::DragStart& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		
		pos = 0;
		neg=0;
	}
	
	void onDoubleClick(const event::DoubleClick& e) override {
		reset();
	}	
	
	void onDragEnd(const event::DragEnd& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
	}
	
	void onDragMove(const event::DragMove& e) override {
		
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		if (e.mouseDelta.x < 0.0f) {
			neg = 0;
			pos++;
		}
		else if (e.mouseDelta.x > 0.0f) {
			pos = 0;
			neg++;
		}
		
		if (neg > 5) {
			paramQuantity->setValue(paramQuantity->getValue() - 1.0f);
			neg = 0;
		}
		else if (pos > 5) {
			paramQuantity->setValue(paramQuantity->getValue() + 1.0f);
			pos = 0;
		}
	}	
#endif

	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgSwitch::randomize();
		
		if (paramQuantity->getValue() > 1.33f)
			paramQuantity->setValue(2.0f);
		else if (paramQuantity->getValue() > 0.67f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// push button base
//-------------------------------------------------------------------
struct CountModulaPB :  SvgSwitch {
	CountModulaPB() {
		// no shadow for switches or buttons
		shadow->opacity = 0.0f;
	}

	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgSwitch::randomize();
		
		if (paramQuantity->getValue() > 0.5f)
			paramQuantity->setValue(1.0f);
		else
			paramQuantity->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// push button
//-------------------------------------------------------------------
struct CountModulaPBSwitch : CountModulaPB {
    CountModulaPBSwitch() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_1.svg")));
    }
};

struct CountModulaPBSwitchMomentary : CountModulaPB {
    CountModulaPBSwitchMomentary() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_1.svg")));

		momentary = true;
    }
};
 
struct CountModulaPBSwitchMomentaryUnlit : CountModulaPB {
    CountModulaPBSwitchMomentaryUnlit() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButton_0.svg")));

		momentary = true;
    }
}; 
 
//-------------------------------------------------------------------
// small square push button
//-------------------------------------------------------------------
struct CountModulaPBSwitchMini : CountModulaPB {
    CountModulaPBSwitchMini() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMini_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMini_1.svg")));
    }
};

struct CountModulaPBSwitchMiniMomentary : CountModulaPB {
    CountModulaPBSwitchMiniMomentary() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMini_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMini_1.svg")));
		
 		momentary = true;
    }
};
 
 

//-------------------------------------------------------------------
// big square push button
//-------------------------------------------------------------------
struct CountModulaPBSwitchBig : CountModulaPB {
    CountModulaPBSwitchBig() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonBig_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonBig_1.svg")));
    }
};

struct CountModulaPBSwitchBigMomentary : CountModulaPB {
    CountModulaPBSwitchBigMomentary() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonBig_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonBig_1.svg")));
		
 		momentary = true;
    }
};
 
//-------------------------------------------------------------------
// really big square push button
//-------------------------------------------------------------------
struct CountModulaPBSwitchMega : CountModulaPB {
    CountModulaPBSwitchMega() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_1.svg")));
    }
};

struct CountModulaPBSwitchMegaMomentary : CountModulaPB {
    CountModulaPBSwitchMegaMomentary() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_1.svg")));

 		momentary = true;
   }
};

struct CountModulaPBSwitchMegaMomentaryUnlit : CountModulaPB {
    CountModulaPBSwitchMegaMomentaryUnlit() {
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/PushButtonMega_0.svg")));
	
 		momentary = true;
   }
};

//-------------------------------------------------------------------
// LED Display
//-------------------------------------------------------------------
struct CountModulaDisplay : TransparentWidget {
	std::shared_ptr<Font> font;
	std::string text;
	float fontSize;
	Vec textPos;
	int numChars = 2;
	
	void setCentredPos(Vec pos) {
		box.pos.x = pos.x - box.size.x/2;
		box.pos.y = pos.y - box.size.y/2;
	}
	
	void draw(const DrawArgs &args) override {
		char buffer[numChars+1];
		int l = text.size();
		if (l > numChars)
			l = numChars;
		
		text.copy(buffer, l);
		buffer[numChars] = '\0';
		
		// Background
		NVGcolor backgroundColor = nvgRGB(0x48, 0x28, 0x28);
		NVGcolor borderColor = nvgRGB(0x10, 0x10, 0x10);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 2.0);
		nvgFillColor(args.vg, backgroundColor);
		nvgFill(args.vg);
		nvgStrokeWidth(args.vg, 1.0);
		nvgStrokeColor(args.vg, borderColor);
		nvgStroke(args.vg);

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
};

struct CountModulaDisplayLarge2 : CountModulaDisplay {
	CountModulaDisplayLarge2() {
		numChars = 2;
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		fontSize = 28;
		box.size = Vec(50, 40);
		textPos = Vec(3, 34);
	}
};

struct CountModulaDisplayLarge3 : CountModulaDisplay {
	CountModulaDisplayLarge3() {
		numChars = 3;
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		fontSize = 28;
		box.size = Vec(75, 40);
		textPos = Vec(3, 34);
	}
};

struct CountModulaDisplayMini2 : CountModulaDisplay {
	CountModulaDisplayMini2() {
		numChars = 2;	
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Segment14.ttf"));
		fontSize = 14;
		box.size = Vec(25, 20);
		textPos = Vec(1, 17);
	}
};