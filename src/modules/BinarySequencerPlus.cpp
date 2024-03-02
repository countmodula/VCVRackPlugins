//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Clock Division Based Sequencer Module
//	Copyright (C) 2024  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/GateProcessor.hpp"

#define NUM_DIVS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BinarySequencerPlus
#define PANEL_FILE "BinarySequencerPlus.svg"

struct BinarySequencerPlus : Module {

	enum ParamIds {
		DIR_PARAM,
		SCALE_PARAM,
		MODE_PARAM,
		LAG_PARAM,
		LAGSHAPE_PARAM,
		ENUMS(DIV_PARAMS, NUM_DIVS),
		GATEMODE_PARAM,
		FLIP_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		RUN_INPUT,
		SH_INPUT,
		DIR_INPUT,
		GATEMODE_INPUT,
		FLIP_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		CLOCK_OUTPUT,
		TRIGGER_OUTPUT,
		CV_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		CLOCK_LIGHT,
		ENUMS(DIV_LIGHTS, NUM_DIVS),
		NUM_LIGHTS
	};
	
	enum CountModes {
		BINARY_MODE,
		BINARY2_MODE,
		DECIMAL_MODE,
		PRIME_MODE,
		FIBONACCI_MODE,
		EVENS_MODE,
		ODDS_MODE,
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
		{2, 3, 5, 8, 13, 21, 34, 55},		// Fibonacci
		{2, 4, 6, 8, 10, 12, 14, 16},		// Even
		{3, 5, 7, 9, 11, 13, 15, 17}		// Odd
	};
	
	const float outputScales[10] = {
		0.0125,		// 1 volt
		0.025,		// 2 volts
		0.0375,		// 3 volts
		0.05,		// 4 volts
		0.0625,		// 5 volts
		0.075,		// 6 volts
		0.0875,		// 7 volts
		0.1,		// 8 volts
		0.1125,		// 9 volts
		0.125		// 10 volts
	};	

	GateProcessor gpClock;
	GateProcessor gpReset;
	GateProcessor gpFlip;
	GateProcessor gpDir;
	GateProcessor gpGatemode;
	GateProcessor gpRun;
	LagProcessor slew;
	dsp::PulseGenerator pgTrig;
	bool countBits[NUM_DIVS] = {};
	
	int mode = 0;
	float flipCV = 0.0f;
	float dirCV = 0.0f;
	float gatemodeCV = 0.0f;
	bool isReset = false;
	int processCount = 8;
	//bool outputClock = true;
	float scale = 0.0;
	
	bool gate = false;
	bool prevGate = false;
	bool trig = false;
	float out = 0.0f;
	float cvParams [NUM_DIVS] = {};
	float lagParamValue = 0.0f;
	float lagShapeValue = 0.0f;	
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BinarySequencerPlus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// switches
		configSwitch(DIR_PARAM, 0.0f, 1.0f, 0.0f, "Count direction", {"Down", "Up"});
		configSwitch(GATEMODE_PARAM, 0.0f, 1.0f, 0.0f, "Gate/Trig mode", {"Clock", "Divisions"});
		configSwitch(MODE_PARAM, 0.0f, (float)maxModes, 0.0f, "Mode", {"Binary 1 (ripple counter)", "Binary 2", "Decimal", "Prime numbers", "Fibonacci", "Even", "Odd"});
		configSwitch(SCALE_PARAM, 1.0f, 10.0f, 10.0f, "Output scale", {"1 Volt", "2 Volts", "3 Volts", "4 Volts", "5 Volts", "6 Volts", "7 Volts", "8 Volts", "9 Volts", "10 Volts" });
		configSwitch(FLIP_PARAM, 0.0f, 1.0f, 0.0f, "Bit flip", {"Off", "On"});
		configParam(LAG_PARAM, 0.0f, 1.0f, 0.0f, "Lag Amount");
		configParam(LAGSHAPE_PARAM, 0.0f, 1.0f, 0.0f, "Lag Shape");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(RUN_INPUT, "Run");
		configInput(SH_INPUT, "Sample & hold");
		configInput(GATEMODE_INPUT, "Gate output mode");
		configInput(DIR_INPUT, "Direction");
		configInput(FLIP_INPUT, "Flip");

		configOutput(CV_OUTPUT, "CV");
		configOutput(INV_OUTPUT, "Inverted CV");
		configOutput(CLOCK_OUTPUT, "Clock");
		configOutput(TRIGGER_OUTPUT, "Trigger");
		
		setParamLabels(true);
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		onReset();
	}

	void setParamLabels(bool configure) {
		// need to wrangle the mode a little where we are in B1 as the output mask works differently
		int m = 1;
		switch (mode) {
			case DECIMAL_MODE:
			case PRIME_MODE:
			case FIBONACCI_MODE:
			case EVENS_MODE:
			case ODDS_MODE:
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
			if (configure) {
				configParam(DIV_PARAMS + i, -10.0f, 10.0f, 0.0f, buffer.str());
			}
			else {
				paramQuantities[DIV_PARAMS + i]->name = buffer.str();
			}
		}
	}


	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "count", json_integer(count));
		json_object_set_new(root, "mode", json_integer(mode));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		return root;
	}

	void dataFromJson(json_t* root) override {

		json_t *cnt = json_object_get(root, "count");
		json_t *m = json_object_get(root, "mode");

		if (cnt)
			count = json_integer_value(cnt);

		if (m) {
			mode = json_integer_value(m);
			setParamLabels(false);
		}

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		gpFlip.reset();
		gpDir.reset();
		gpGatemode.reset();		
		gpRun.reset();		
		
		slew.reset();
		
		pgTrig.reset();
		
		count = 512; // reset will set the mode back to binary
		mode = 0;
		isReset = true;
		processCount = 8;
		//outputClock = true;
		flipCV = 0.0f;
		dirCV = 0.0f;
		gatemodeCV = 0.0f;
		gate = prevGate = false;
		scale = 0.0f;
		lagParamValue = 0.0f;
		lagShapeValue = 0.0f;

		for (int c = 0; c < NUM_DIVS; c++) {
			cvParams [c] = 0.0f;
			countBits[c] = false;
		}		
	}
	
	void process(const ProcessArgs &args) override {

		if (++processCount > 8) {
			processCount = 0;
			
			// what count mode are we using?
			int prevMode = mode;
			mode = clamp((int)params[MODE_PARAM].getValue(), 0, maxModes);
			
			if (mode != prevMode) {
				setParamLabels(false);
			}
			
			// grab the count direction param value
			dirCV = params[DIR_PARAM].getValue() * 10.0f;

			// grab the gatemode param value
			gatemodeCV = params[GATEMODE_PARAM].getValue() * 10.0f;
			
			// grab the flip param value
			flipCV = params[FLIP_PARAM].getValue() * 10.0f;
			
			// what scale do we have selected?
			scale = outputScales[clamp((int)(params[SCALE_PARAM].getValue())-1, 0, 10)];	

			// grab the cv parametervalues
			for (int i = 0; i < NUM_DIVS; i++ ) {
				cvParams [i] = params[DIV_PARAMS +i].getValue();
			}
			
			lagParamValue = params[LAG_PARAM].getValue();
			lagShapeValue = params[LAGSHAPE_PARAM].getValue();
		}
		
		// process the CV Inputs
		bool flip = gpFlip.set(inputs[FLIP_INPUT].getNormalVoltage(flipCV));
		bool outputClock = !gpGatemode.set(inputs[GATEMODE_INPUT].getNormalVoltage(gatemodeCV));
		bool countUp = gpDir.set(inputs[DIR_INPUT].getNormalVoltage(dirCV));
	
		// process the run input
		bool run = gpRun.set(inputs[RUN_INPUT].getNormalVoltage(10.0f));
		
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
			if (run && gpClock.leadingEdge()) {
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
		
		// calculate the cv value, set the lights and outputs
		out = 0.0f;
		prevGate = gate;
		gate = outputClock;
		int b = flip ? 7 : 0;
		for (int c = 0; c < NUM_DIVS; c++) {
			if (isReset) {
				countBits[b] = false;
			}
			else if (mode == BINARY_MODE) {
				countBits[b] = ((count & outputMask[mode][c]) > 0);
			}
			else {
				countBits[b] = ((count % outputMask[mode][c]) == 0);
			}
	
			if (countBits[b]) {
				lights[DIV_LIGHTS + b].setBrightness(1.0f);

				out += cvParams[DIV_PARAMS + b];
				if (!outputClock) {
					gate = true;
				}
			}
			else {
				lights[DIV_LIGHTS + b].setBrightness(0.0f);
			}
			
			if (flip) {
				b--;
			}
			else {
				b++;
			}
		}
		
		// process the output trigger
		if (prevGate != gate && gate && gpClock.leadingEdge()) {
			trig = true;
			pgTrig.trigger(1e-3f);
		}
		else {
			trig = pgTrig.process(args.sampleTime);	
		}
		
		// scale the cv output value to the currently selected range and apply lag
		out = slew.process(out *= scale, lagShapeValue, lagParamValue, lagParamValue, args.sampleTime);
			
		outputs[CLOCK_OUTPUT].setVoltage(boolToGate(run && gate && gpClock.high()));
		outputs[TRIGGER_OUTPUT].setVoltage(boolToGate(trig));
		outputs[CV_OUTPUT].setVoltage(out);
		outputs[INV_OUTPUT].setVoltage(-out);
	}
};

struct BinarySequencerPlusWidget : ModuleWidget {

	std::string panelName;
	
	const int CUSTOM_ROWS[6] = {
		73,
		142,
		192,
		242,
		292,
		342
	};
	
	const int CUSTOM_HALF_ROW3 = 217;
	
	BinarySequencerPlusWidget(BinarySequencerPlus *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS[STD_ROW2]), module, BinarySequencerPlus::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS[STD_ROW2]), module, BinarySequencerPlus::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], CUSTOM_ROWS[STD_ROW2]), module, BinarySequencerPlus::RUN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], CUSTOM_ROWS[STD_ROW2]), module, BinarySequencerPlus::SH_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS[STD_ROW3]), module, BinarySequencerPlus::DIR_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS[STD_ROW4]), module, BinarySequencerPlus::FLIP_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS[STD_ROW5]), module, BinarySequencerPlus::GATEMODE_INPUT));
	
		
		// division lights and knobs
		float p = STD_COLUMN_POSITIONS[STD_COL1];
		float d = (STD_COLUMN_POSITIONS[STD_COL7] - p)/7.0;
		for (int s = 0; s < NUM_DIVS; s++) {
				addParam(createLightParamCentered<VCVLightSlider<GreenLight>>(Vec(p, CUSTOM_ROWS[STD_ROW1]), module, BinarySequencerPlus::DIV_PARAMS + s, BinarySequencerPlus::DIV_LIGHTS + s));
				p += d;
		}

		// other knobs and switches controls
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], CUSTOM_HALF_ROW3), module, BinarySequencerPlus::MODE_PARAM));
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], CUSTOM_HALF_ROW3), module, BinarySequencerPlus::SCALE_PARAM));
		
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS[STD_ROW3]), module, BinarySequencerPlus::DIR_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], CUSTOM_ROWS[STD_ROW5]), module, BinarySequencerPlus::LAG_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], CUSTOM_ROWS[STD_ROW5]), module, BinarySequencerPlus::LAGSHAPE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS[STD_ROW4]), module, BinarySequencerPlus::FLIP_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS[STD_ROW5]), module, BinarySequencerPlus::GATEMODE_PARAM));

		// outputs 
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], CUSTOM_ROWS[STD_ROW6]), module, BinarySequencerPlus::CLOCK_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], CUSTOM_ROWS[STD_ROW6]), module, BinarySequencerPlus::TRIGGER_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], CUSTOM_ROWS[STD_ROW6]), module, BinarySequencerPlus::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], CUSTOM_ROWS[STD_ROW6]), module, BinarySequencerPlus::INV_OUTPUT));
	}	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	struct InitMenuItem : MenuItem {
		BinarySequencerPlusWidget *widget;
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize division mix";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < NUM_DIVS; i ++) {
				widget->getParam(BinarySequencerPlus::DIV_PARAMS + i)->getParamQuantity()->reset();
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize division mix";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < NUM_DIVS; i ++) {
				widget->getParam(BinarySequencerPlus::DIV_PARAMS + i)->getParamQuantity()->randomize();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);				}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	void doRandom() {
		// history - current settings
		history::ModuleChange *h = new history::ModuleChange;
		h->name = "randomize division mix";
		h->moduleId = this->module->id;
		h->oldModuleJ = this->toJson();
	
		for (int i = 0; i < NUM_DIVS; i ++) {
			this->getParam(BinarySequencerPlus::DIV_PARAMS + i)->getParamQuantity()->randomize();
		}

		// history - new settings
		h->newModuleJ = this->toJson();
		APP->history->push(h);			
	}
	
	struct RandMenuItem : MenuItem {
		BinarySequencerPlusWidget *widget;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
			widget->doRandom();
		}
	};

	void appendContextMenu(Menu *menu) override {
		BinarySequencerPlus *module = dynamic_cast<BinarySequencerPlus*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));
		
		// CV only init
		InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("Initialize Division Mix Only");
		initCVMenuItem->widget = this;
		menu->addChild(initCVMenuItem);

		// CV only random
		RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("Randomize Division Mix Only", "Shift+Ctrl+M");
		randCVMenuItem->widget = this;
		menu->addChild(randCVMenuItem);
	}	
	
	void onHoverKey(const event::HoverKey &e) override {
		if (e.action == GLFW_PRESS && (e.mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			
			switch (e.key) {
				case GLFW_KEY_M:
					doRandom();
					e.consume(this);
			}
		}

		ModuleWidget::onHoverKey(e);
	}		
	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};

Model *modelBinarySequencerPlus = createModel<BinarySequencerPlus, BinarySequencerPlusWidget>("BinarySequencerPlus");
