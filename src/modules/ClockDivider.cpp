//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Clock Divider
//	A multimode clock divider
//	Copyright (C) 2021  Adam Verspaget
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
		DIR_PARAM,
		TRIG_PARAM,
		MODE_PARAM,
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
	
	
	enum CountModes {
		BINARY_MODE,
		BINARY2_MODE,
		DECIMAL_MODE,
		PRIME_MODE,
#ifdef THE_FULL_MONTY
		FIBONACCI_MODE,
		EVENS_MODE,
		ODDS_MODE,
#endif
		NUM_MODES
	};
	
	
	const int maxModes = NUM_MODES-1;
	
	int count = 512;
	const int maxCount[7] = {512, 512, 362880, 9699690, 122522400, 10321920, 34459425};
	const int maxCountMinusOne [7] = {511, 511, 362879, 9699689, 122522399, 10321919, 34459424 };

	const int outputMask[7][8] = {
		{1, 2, 4, 8, 16, 32, 64, 128},		// B1
		{2, 4, 8, 16, 32, 64, 128, 256},	// B2
		{2, 3, 4, 5, 6, 7, 8, 9 },			// Decimal
		{2, 3, 5, 7, 11, 13, 17, 19},		// Prime
		// will acutally use the following when I can figure out a nice way to display the divisions
		{2, 3, 5, 8, 13, 21, 34, 55},		// Fibonacci
		{2, 4, 6, 8, 10, 12, 14, 16},		// Even
		{3, 5, 7, 9, 11, 13, 15, 17}		// Odd
	};

	GateProcessor gpClock;
	GateProcessor gpReset;

	dsp::PulseGenerator pgDiv[NUM_DIVS];
	bool countBits[NUM_DIVS] = {};
	
	bool countUp = false;
	bool doTrigs = false;
	int mode = 0;
	bool isReset = false;
	int processCount = 8;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ClockDivider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// switches
		configSwitch(DIR_PARAM, 0.0f, 1.0f, 0.0f, "Count direction", {"Down (more musical)", "Up (not so musical)"});
		configSwitch(TRIG_PARAM, 0.0f, 1.0f, 0.0f, "Output mode", {"Gates", "Triggers"});
		configSwitch(MODE_PARAM, 0.0f, (float)maxModes, 0.0f, "Mode", {"Binary 1 (ripple counter)", "Binary 2", "Decimal", "Prime numbers"});

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");

		setOutputLabels();
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		mode = 0;
		processCount = 8;
		countUp = false;
		isReset = false;
	}

	void setOutputLabels() {
		// need to wrangle the mode a little where we are in B1 as the output mask works differently
		int m = 1;
		switch (mode) {
			case DECIMAL_MODE:
			case PRIME_MODE:
	#ifdef THE_FULL_MONTY
			case FIBONACCI_MODE:
			case EVENS_MODE:
			case ODDS_MODE:
	#endif
				m = mode;
				break;
			default:
				m = 1;
				break;
		}

		std::ostringstream  buffer;
		for (int i = 0; i < NUM_DIVS; i++) {
			buffer.str("");
			buffer << "Divide by " << outputMask[m][i];
			configOutput(DIV_OUTPUTS + i, buffer.str());
		}
	}


	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "count", json_integer(count));
		json_object_set_new(root, "mode", json_integer(mode));
		json_object_set_new(root, "doTrigs", json_boolean(doTrigs));
		json_object_set_new(root, "countUp", json_boolean(countUp));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		return root;
	}

	void dataFromJson(json_t* root) override {

		json_t *cnt = json_object_get(root, "count");
		json_t *m = json_object_get(root, "mode");
		json_t *trigs = json_object_get(root, "doTrigs");
		json_t *cu = json_object_get(root, "countUp");

		if (cnt)
			count = json_integer_value(cnt);

		if (m) {
			mode = json_integer_value(m);
			setOutputLabels();
		}

		if (trigs)
			doTrigs = json_boolean_value(trigs);

		if (cu)
			countUp = json_boolean_value(cu);
			
		// stop triggers from firing on startup
		if (doTrigs)
			isReset = true;

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		count = 512; // reset will set the mode back to binary
		isReset = true;
		processCount = 8;
		
		for (int c = 0; c < NUM_DIVS; c++) {
			countBits[c] = false;
			pgDiv[c].reset();
		}
	}
	
	void process(const ProcessArgs &args) override {

		if (++processCount > 8) {
			processCount = 0;
			countUp = (params[DIR_PARAM].getValue() > 0.5f);
			doTrigs = (params[TRIG_PARAM].getValue() > 0.5f);
			int prevMode = mode;
			mode = clamp((int)params[MODE_PARAM].getValue(), 0, maxModes);
			
			if (mode != prevMode)
				setOutputLabels();
		}
		
		// process the reset
		gpReset.set(inputs[RESET_INPUT].getVoltage());
		
		if (gpReset.leadingEdge()) {
			isReset = true;
			if (mode == BINARY_MODE)
				count = (countUp ? 0 : maxCount[mode]);
			else
				count = (countUp ? 0 : 1);
		}
		else {
			// process the clock
			gpClock.set(inputs[CLOCK_INPUT].getVoltage());
			if (gpClock.leadingEdge()) {
				isReset = false;
				if (countUp) {
					if (++count > maxCountMinusOne[mode])
						count = 0;
				}
				else {
					if (--count < 1)
						count = maxCount[mode];
				}
			}
		}
		
		// set lights and outputs
		bool divActive;
		for (int c = 0; c < NUM_DIVS; c++) {
			divActive = countBits[c];
			
			if (isReset)
				countBits[c] = false;
			else if (mode == BINARY_MODE)
				countBits[c] = ((count & outputMask[mode][c]) > 0);
			else
				countBits[c] = ((count % outputMask[mode][c]) == 0);
	
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
		}
	}
};

struct ClockDividerWidget : ModuleWidget {

	std::string panelName;
	
	ClockDividerWidget(ClockDivider *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		// clock and reset inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, ClockDivider::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, ClockDivider::RESET_INPUT));
		
		
		// row lights and knobs
		for (int s = 0; s < NUM_DIVS; s++) {
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 19, STD_ROWS8[STD_ROW1 + s]), module, ClockDivider::DIV_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, ClockDivider::DIV_OUTPUTS + s));
		}

		// count direction control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, ClockDivider::DIR_PARAM));

		// output mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, ClockDivider::TRIG_PARAM));
		
		// mode control
		addParam(createParamCentered<RotarySwitch<OperatingAngle120<Left90<SmallKnob<BlueKnob>>>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, ClockDivider::MODE_PARAM));

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
