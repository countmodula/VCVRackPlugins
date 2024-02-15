//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Custom components
//  Copyright (C) 2021  Adam Verspaget 
//----------------------------------------------------------------------------
#include "componentlibrary.hpp"

using namespace rack;

//-------------------------------------------------------------------
// screws
//-------------------------------------------------------------------
struct CountModulaScrew : SvgScrew {
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

struct CountModulaLightRB : GrayModuleLightWidget {
	CountModulaLightRB() {
		addBaseColor(SCHEME_RED);
		addBaseColor(SCHEME_BLUE);
	}
};

/** Based on the size of 3mm LEDs */
template <typename TBase>
struct MediumLightSquare : TBase {
	 MediumLightSquare() {
		this->box.size = rack::window::mm2px(math::Vec(3.176, 3.176));
	}
};

/** Based on the size of 3mm LEDs */
template <typename TBase>
struct MediumLightRectangle : TBase {
	 MediumLightRectangle() {
		this->box.size = rack::window::mm2px(math::Vec(6.352, 3.176));
	}
};

template <typename TBase>
struct CountModulaRectangleLight : TBase {
	void drawBackground(const widget::Widget::DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);

		// Background
		if (this->bgColor.a > 0.0) {
			nvgFillColor(args.vg, this->bgColor);
			nvgFill(args.vg);
		}
		
		// Border
		if (this->borderColor.a > 0.0) {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, this->borderColor);
			nvgStroke(args.vg);
		}		
	}
	
	void drawLight(const widget::Widget::DrawArgs& args) override {

		// Foreground
		if (this->color.a > 0.0) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);
			
			nvgFillColor(args.vg, this->color);
			nvgFill(args.vg);
		}
	}
};

template <typename TBase>
struct CountModulaSquareLight : TBase {

	void drawBackground(const widget::Widget::DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);

		// Background
		if (this->bgColor.a > 0.0) {
			nvgFillColor(args.vg, this->bgColor);
			nvgFill(args.vg);
		}
		
		// Border
		if (this->borderColor.a > 0.0) {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, this->borderColor);
			nvgStroke(args.vg);
		}
	}

	void drawLight(const widget::Widget::DrawArgs& args) override {
		// Foreground
		if (this->color.a > 0.0) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);
			nvgFillColor(args.vg, this->color);
			nvgFill(args.vg);
		}

	}
};

//-------------------------------------------------------------------
// Ports
//-------------------------------------------------------------------
struct CountModulaJack : SvgPort {
	CountModulaJack() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Jack.svg")));
	}
};

struct CountModulaJackNoNut : SvgPort {
	CountModulaJackNoNut() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/JackNoNut.svg")));
	}
};

//-------------------------------------------------------------------
// Rotary controls
//-------------------------------------------------------------------
// Rotary switch base - values are limted to whole numbers
template <typename TBase>
struct RotarySwitch : TBase {
	RotarySwitch() {
		this->snap = true;
		this->smooth = false;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgKnob::onChange(e);
		
		this->getParamQuantity()->setValue(roundf(this->getParamQuantity()->getValue()));
	}
};

// standard rotary potentiometer base
template <typename TBase>
struct Potentiometer : TBase {
	Potentiometer() {
	}
};


//-------------------------------------------------------------------
// on-off toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle2P : SvgSwitch {
	CountModulaToggle2P() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_2.svg")));

		// no shadow for switches
		shadow->opacity = 0.0f;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);

		if (getParamQuantity()->getValue() > 0.5f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// on-off toggle switch - sideways
//-------------------------------------------------------------------
struct CountModulaToggle2P90 : SvgSwitch {
	CountModulaToggle2P90() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));

		// no shadow for switches
		shadow->opacity = 0.0f;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		if (getParamQuantity()->getValue() > 0.5f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// on-off-on toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle3P : SvgSwitch {
	CountModulaToggle3P() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_Toggle_2.svg")));
		
		// no shadow for switches
		shadow->opacity = 0.0f;
	}

	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		float v = getParamQuantity()->getValue();
		
		if (v > 1.33f)
			getParamQuantity()->setValue(2.0f);
		else if (v > 0.67f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// on-off-on toggle switch - sideways
//-------------------------------------------------------------------
struct CountModulaToggle3P90 : SvgSwitch {
	CountModulaToggle3P90() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));
		
		// no shadow for switches
		shadow->opacity = 0.0f;
	}

	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		float v = getParamQuantity()->getValue();
		
		if (v > 1.33f)
			getParamQuantity()->setValue(2.0f);
		else if (v > 0.67f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};

struct CountModulaToggle3P270 : SvgSwitch {
	CountModulaToggle3P270() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		
		// no shadow for switches
		shadow->opacity = 0.0f;
	}

	// handle the manually entered values
	void onChange(const event::Change &e) override {
		
		SvgSwitch::onChange(e);
		
		float v = getParamQuantity()->getValue();
		if (v > 1.33f)
			getParamQuantity()->setValue(2.0f);
		else if (v > 0.67f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};