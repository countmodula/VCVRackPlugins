//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Clock/Gate Module
//	A voltage controlled clock/gate divider (divide by 1 - approx 20)
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME VCPulseDivider
#define PANEL_FILE "VCPulseDivider.svg"

struct VCPulseDivider : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		DIV_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DIV1_OUTPUT,
		DIVN_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		IN_LIGHT,
		DIV1_LIGHT,
		DIVN_LIGHT,
		NUM_LIGHTS
	};

	char lengthString[4];
	GateProcessor gateClock;
	GateProcessor gateReset;

	int count = 0; 
	int length = 1;

	float reset, clock, div, divCV;

	short processCount = 8;
	bool out1 = false;
	bool outN = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	VCPulseDivider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// scale 10V CV up to 32
		configParam(CV_PARAM, -3.2f, 3.2f, 0.0f, "CV Amount", " %", 0.0f, 31.25f, 0.0f);
		configParam(MANUAL_PARAM, 1.0f, 32.0f, 0.0f, "Divide by");
		
		configInput(DIV_INPUT, "Pulse");
		configInput(RESET_INPUT, "Reset");
		configInput(CV_INPUT, "Division CV");
		
		configOutput(DIV1_OUTPUT, "On the 1");
		configOutput(DIVN_OUTPUT, "On the N");

		outputInfos[DIV1_OUTPUT]->description = "Output pulse occurs on the first clock of the division cycle";
		outputInfos[DIVN_OUTPUT]->description = "Output pulse occurs on the last clock of the division cycle";
		
		processCount = 8;
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		count = 0;
		length = 1;
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void process(const ProcessArgs &args) override {

		// reset input
		gateReset.set(inputs[RESET_INPUT].getNormalVoltage(0.0f));
		
		// clock
		gateClock.set(inputs[DIV_INPUT].getNormalVoltage(0.0f));
		
		// division
		div = params[MANUAL_PARAM].getValue();
		divCV = params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f);
		length = (int)(clamp(div + divCV, 1.0f, 32.0f));
		
		if (gateReset.leadingEdge()) {
			// reset the count at zero
			count = 0;
		}
		
		// increment count on positive clock edge
		if (gateClock.leadingEdge()){
			count++;
			
			if (count > length)
				count = 1;
		}

		// are we at or over the current div setting?
		outN = (gateClock.high() && count >= length);
		
		// are we on the first pulse?
		out1 = (gateClock.high() && count == 1);
		
		if (++processCount > 8) {
			processCount = 0;

			lights[IN_LIGHT].setBrightness(boolToLight(gateClock.high()));
			lights[DIV1_LIGHT].setBrightness(boolToLight(out1));
			lights[DIVN_LIGHT].setBrightness(boolToLight(outN));
		}
		
		outputs[DIV1_OUTPUT].setVoltage(boolToGate(out1));
		outputs[DIVN_OUTPUT].setVoltage(boolToGate(outN));
	}
};

struct VCPulseDividerWidget : ModuleWidget {
	std::string panelName;
	CountModulaLEDDisplayLarge2 *divDisplay;
	
	VCPulseDividerWidget(VCPulseDivider *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, VCPulseDivider::CV_PARAM));
		addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, VCPulseDivider::MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCPulseDivider::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCPulseDivider::DIV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCPulseDivider::RESET_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCPulseDivider::DIV1_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCPulseDivider::DIVN_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, VCPulseDivider::IN_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, VCPulseDivider::DIV1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, VCPulseDivider::DIVN_LIGHT));
	
		// LED display
		divDisplay = new CountModulaLEDDisplayLarge2();
		divDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW1]));
		divDisplay->text =  "01";
		addChild(divDisplay);
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		VCPulseDivider *module = dynamic_cast<VCPulseDivider*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			VCPulseDivider *m = (VCPulseDivider *)module;
				
			divDisplay->text = rack::string::f("%02d", m->length);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelVCPulseDivider = createModel<VCPulseDivider, VCPulseDividerWidget>("VCPulseDivider");
