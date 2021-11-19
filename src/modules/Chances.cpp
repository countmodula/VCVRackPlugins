//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Chances (Bernouli Gate)
//	Copyright (C) 2019  Adam Verspaget
//	Logic portions taken from Branches (Bernoulli Gate) by Andrew Belt
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Chances
#define PANEL_FILE "Chances.svg"

struct Chances : Module {
	enum ParamIds {
		THRESH_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		PROB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A_LIGHT,
		B_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gateTriggers;
	float moduleVersion = 1.1f;	
	
	bool latch = false;
	bool toggle = false;
	bool outcome = true;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	Chances() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(THRESH_PARAM, 0.0f, 1.0f, 0.5f, "Output B chance", "%", 0.0f, 100.0f, 0.0f);
		configSwitch(MODE_PARAM, 0.0f, 2.0f, 1.0f, "Mode", {"Toggle", "Normal", "Latch"});

		configInput(GATE_INPUT, "Gate");
		configInput(PROB_INPUT, "Probability CV");
		
		configOutput(A_OUTPUT, "A");
		configOutput(B_OUTPUT, "B");

		configBypass(GATE_INPUT, A_OUTPUT);
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gateTriggers.reset();
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();
		json_object_set_new(root, "moduleVersion", json_real(moduleVersion));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		json_t *version = json_object_get(root, "moduleVersion");

		if (version)
			moduleVersion = json_number_value(version);		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		// older module version, convert the pushbutton value to the equivalent toggle value
		// and update to the latest version
		if (moduleVersion < 1.1f) {
			moduleVersion = 1.1f;	
			params[MODE_PARAM].setValue(params[MODE_PARAM].getValue() + 1.0f);
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		// process the gate input
		gateTriggers.set(inputs[GATE_INPUT].getVoltage());
		
		// what mode are we in?
		switch ((int)(params[MODE_PARAM].getValue())) {
			case 2:
				latch = true;
				toggle = false;
				break;
			 case 0:
				latch = false;
				toggle = true;
				break;
			 default:
				latch = false;
				toggle = false;
				break;
		}
		
		// determine which output we're going to use
		if (gateTriggers.leadingEdge()) {
			float r = random::uniform();
			float threshold = clamp(params[THRESH_PARAM].getValue() + inputs[PROB_INPUT].getVoltage() / 10.f, 0.f, 1.f);

			// toggle mode only changes when the outcome is different to the last outcome
			if (toggle)
				outcome = (outcome != (r < threshold));
			else
				outcome = (r < threshold);
		}

		// in latch mode, the outputs should just flip between themselves based on the outcome rather than following the gate input
		bool gate = latch || gateTriggers.high();
		bool b = outcome && gate;
		bool a = !outcome && gate;
		
		if (a || b) {
			// just flip the lights if we have one or the other
			lights[A_LIGHT].setBrightness(boolToLight(a));
			lights[B_LIGHT].setBrightness(boolToLight(b));
		}
		else{
			// fade the lights if we've got nothing going on
			lights[A_LIGHT].setSmoothBrightness(boolToLight(a), args.sampleTime);
			lights[B_LIGHT].setSmoothBrightness(boolToLight(b), args.sampleTime);
		}
		
		outputs[A_OUTPUT].setVoltage(boolToGate(a));
		outputs[B_OUTPUT].setVoltage(boolToGate(b));
	}
};

struct ChancesWidget : ModuleWidget {

	std::string panelName;
	
	ChancesWidget(Chances *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, Chances::THRESH_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4] - 10), module, Chances::MODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, Chances::PROB_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, Chances::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Chances::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, Chances::B_OUTPUT));
		
		// outcome lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1]- 10, STD_ROWS7[STD_ROW3]), module, Chances::A_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1]+ 10, STD_ROWS7[STD_ROW3]), module, Chances::B_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Chances *module = dynamic_cast<Chances*>(this->module);
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

Model *modelChances = createModel<Chances, ChancesWidget>("Chances");
