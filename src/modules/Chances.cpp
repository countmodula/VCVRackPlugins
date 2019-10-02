//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Chances (Bernouli Gate)
//  Copyright (C) 2019  Adam Verspaget
//  Logic portions taken from Branches (Bernoulli Gate) by Andrew Belt
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
		ENUMS(STATE_LIGHT, 2),
		NUM_LIGHTS
	};

	GateProcessor gateTriggers;
	
	bool latch = false;
	bool outcome = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	Chances() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(THRESH_PARAM, 0.0f, 1.0f, 0.5f, "Chance");
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Latch mode");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gateTriggers.reset();
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();
		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void process(const ProcessArgs &args) override {
		
		// process the gate input
		gateTriggers.set(inputs[GATE_INPUT].getVoltage());
		
		// what mode are we in?
		latch = params[MODE_PARAM].getValue() > 0.5f;

		// determine which output we're going to use
		if (gateTriggers.leadingEdge()) {
			float r = random::uniform();
			float threshold = clamp(params[THRESH_PARAM].getValue() + inputs[PROB_INPUT].getVoltage() / 10.f, 0.f, 1.f);
			outcome = (r < threshold);
		}

		// in latch mode, the outputs should just flip between themselves based on the outcome rather than following the gate input
		bool gate = latch || gateTriggers.high();
		bool a = outcome && gate;
		bool b = !outcome && gate;
		
		if (a || b) {
			// just flip the lights if we have one or the other
			lights[STATE_LIGHT].setBrightness(boolToLight(a));
			lights[STATE_LIGHT+1].setBrightness(boolToLight(b));
		}
		else{
			// fade the lights if we've got nothing going on
			lights[STATE_LIGHT].setSmoothBrightness(boolToLight(a), args.sampleTime);
			lights[STATE_LIGHT+1].setSmoothBrightness(boolToLight(b), args.sampleTime);
		}
		
		outputs[A_OUTPUT].setVoltage(boolToGate(a));
		outputs[B_OUTPUT].setVoltage(boolToGate(b));
	}
};

struct ChancesWidget : ModuleWidget {
	ChancesWidget(Chances *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Chances.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, Chances::THRESH_PARAM));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, Chances::MODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, Chances::PROB_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, Chances::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Chances::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, Chances::B_OUTPUT));
		
		// polarity lights
		addChild(createLightCentered<MediumLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, Chances::STATE_LIGHT));
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
