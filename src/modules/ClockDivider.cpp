//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define NUM_DIVS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ClockDivider
#define PANEL_FILE "ClockDivider.svg"


struct ClockDivider : Module {

	enum ParamIds {
		MODE_PARAM,
		TRIG_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		ENUMS(DIV_OUTPUTS, NUM_DIVS),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(DIV_LIGHTS, NUM_DIVS),
		NUM_LIGHTS
	};
	
	short count = 512;
	GateProcessor gpClock;
	GateProcessor gpReset;

	dsp::PulseGenerator pgDiv[NUM_DIVS];
	bool countBits[NUM_DIVS] = {};
	
	short bitMask;
	bool countUp = false;;
	bool doTrigs = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ClockDivider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// mode switch
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Count mode");
		
		// trig switch
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Output mode");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "count", json_integer(count));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
			
		return root;
	}

	void dataFromJson(json_t* root) override {
		
		json_t *cnt = json_object_get(root, "count");
		
		if (cnt)
			count = json_integer_value(cnt);
	
	
		short bitMask = 0x01;
		
		for (int c = 0; c < NUM_DIVS; c++) {
			countBits[c] = ((count & bitMask) == bitMask);
		
			// prepare for next bit
			bitMask = bitMask << 1;
		}
	
	
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		count = 512;
		
		for (int c = 0; c < NUM_DIVS; c++) {
			countBits[c] = false;
			pgDiv[c].reset();
		}
	}
	
	void process(const ProcessArgs &args) override {

		countUp = (params[MODE_PARAM].getValue() > 0.5f);
		doTrigs = (params[TRIG_PARAM].getValue() > 0.5f);
	
		// process the reset
		gpReset.set(inputs[RESET_INPUT].getVoltage());
		if (gpReset.leadingEdge()) {
			count = (countUp ? 0 : 512);
			
			for (int c = 0; c < NUM_DIVS; c++) 
				countBits[c] = false;	
		}
		
		// process the clock
		gpClock.set(inputs[CLOCK_INPUT].getVoltage());
		if (gpClock.leadingEdge()) {
			if (countUp) {
				if (++count > 511)
					count = 0;
			}
			else {
				if (--count < 1)
					count = 512;
			}
		}
		
		// set lights and outputs
		bitMask = 0x01;
		for (int c = 0; c < NUM_DIVS; c++) {

			bool divActive = countBits[c];
			countBits[c] = ((count & bitMask) == bitMask);
		
			if (doTrigs) {
				if (countBits[c] && !divActive) {
					divActive = true;
					pgDiv[c].trigger(1e-3f);
				}
				else {
					divActive = pgDiv[c].process(args.sampleTime);
				}
				
				lights[DIV_LIGHTS + c].setSmoothBrightness(boolToLight(divActive), args.sampleTime);
			}
			else {
				divActive = countBits[c];
				lights[DIV_LIGHTS + c].setBrightness(boolToLight(divActive));

				// ensure any residual triggers are processed
				pgDiv[c].process(args.sampleTime);	
			}
		
			outputs[DIV_OUTPUTS + c].setVoltage(boolToGate(divActive));
			
			// prepare for next bit
			bitMask = bitMask << 1;
		}
	}
};

struct ClockDividerWidget : ModuleWidget {
	ClockDividerWidget(ClockDivider *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ClockDivider.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, ClockDivider::CLOCK_INPUT));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, ClockDivider::RESET_INPUT));
		
		
		// row lights and knobs
		for (int s = 0; s < NUM_DIVS; s++) {
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 19, STD_ROWS8[STD_ROW1 + s]), module, ClockDivider::DIV_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, ClockDivider::DIV_OUTPUTS + s));
		}

		// count mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, ClockDivider::MODE_PARAM));

		// trig mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, ClockDivider::TRIG_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ClockDivider *module = dynamic_cast<ClockDivider*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};

Model *modelClockDivider = createModel<ClockDivider, ClockDividerWidget>("ClockDivider");
