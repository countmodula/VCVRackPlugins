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

template <typename TBase>
struct CountModulaRectangleLight : TBase {
	void drawLight(const widget::Widget::DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, this->box.size.x * 2, this->box.size.y);

		// Background
		if (this->bgColor.a > 0.0) {
			nvgFillColor(args.vg, this->bgColor);
			nvgFill(args.vg);
		}

		// Foreground
		if (this->color.a > 0.0) {
			nvgFillColor(args.vg, this->color);
			nvgFill(args.vg);
		}

		// Border
		if (this->borderColor.a > 0.0) {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, this->borderColor);
			nvgStroke(args.vg);
		}
	}
};

template <typename TBase>
struct CountModulaSquareLight : TBase {
	void drawLight(const widget::Widget::DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);

		// Background
		if (this->bgColor.a > 0.0) {
			nvgFillColor(args.vg, this->bgColor);
			nvgFill(args.vg);
		}

		// Foreground
		if (this->color.a > 0.0) {
			nvgFillColor(args.vg, this->color);
			nvgFill(args.vg);
		}

		// Border
		if (this->borderColor.a > 0.0) {
			nvgStrokeWidth(args.vg, 0.5);
			nvgStrokeColor(args.vg, this->borderColor);
			nvgStroke(args.vg);
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
		
		this->paramQuantity->setValue(roundf(this->paramQuantity->getValue()));
	}
	
	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SvgKnob::randomize();
		
		this->paramQuantity->setValue(roundf(this->paramQuantity->getValue()));
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

struct CountModulaToggle3P270 : SvgSwitch {
	int pos;
	int neg;
	
	CountModulaToggle3P270() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_2.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/SW_ToggleS_0.svg")));
		
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