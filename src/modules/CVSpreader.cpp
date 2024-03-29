//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - CV Spreader Module
//	Based on the CGS37 CV Cluster by Ken Stone.
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME CVSpreader
#define PANEL_FILE "CVSpreader.svg"

struct CVSpreader : Module {
	enum ParamIds {
		BASE_PARAM,
		SPREAD_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		BASE_INPUT,
		SPREAD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		E_OUTPUT,
		F_OUTPUT,
		G_OUTPUT,
		H_OUTPUT,
		I_OUTPUT,
		J_OUTPUT,
		K_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	CVSpreader() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(BASE_PARAM, -1.0f, 1.0f, 0.0f, "Base CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(SPREAD_PARAM, -1.0f, 1.0f, 0.0f, "Spread CV amount", " %", 0.0f, 100.0f, 0.0f);
		configSwitch(MODE_PARAM, 0.0f, 1.0f, 1.0f, "Spread", {"Even", "Odd"});

		configInput(BASE_INPUT, "Base CV");
		configInput(SPREAD_INPUT, "Spread CV");

		configOutput(A_OUTPUT, "Sum 1");
		configOutput(B_OUTPUT, "Sum 2");
		configOutput(C_OUTPUT, "Sum 3");
		configOutput(D_OUTPUT, "Sum 4");
		configOutput(E_OUTPUT, "Sum 5");
		configOutput(F_OUTPUT, "Centre");
		configOutput(G_OUTPUT, "Difference 1");
		configOutput(H_OUTPUT, "Difference 2");
		configOutput(I_OUTPUT, "Difference 3");
		configOutput(J_OUTPUT, "Difference 4");
		configOutput(K_OUTPUT, "Difference 5");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));

		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
	
	void process(const ProcessArgs &args) override {

		float base = inputs[BASE_INPUT].getNormalVoltage(10.0f) * params[BASE_PARAM].getValue();
		float spread = inputs[SPREAD_INPUT].getNormalVoltage(5.0f) * params[SPREAD_PARAM].getValue();
		float even = (params[MODE_PARAM].getValue() < 0.5f ? 0.0f : 1.0f);
		
		float pos = base + spread;
		float neg = base - spread;
		float diff = 2.0f * spread;
		float div = diff / (9.0f + (even ? 1.0f : 0.0f));

		// output F is always the base value
		outputs[F_OUTPUT].setVoltage(base);
		
		for (int i = 0; i < 5; i++) {
			// pos outputs
			outputs[E_OUTPUT - i].setVoltage(clamp(pos - ((float)i * div), -10.0f, 10.0f));
			
			// neg outputs
			outputs[K_OUTPUT - i].setVoltage(clamp(neg + ((float)i * div), -10.0f, 10.0f));
		}
	}		
};

struct CVSpreaderWidget : ModuleWidget {

	std::string panelName;
	
	CVSpreaderWidget(CVSpreader *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// CV knobs
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, CVSpreader::BASE_PARAM));
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, CVSpreader::SPREAD_PARAM));
	
		// odd/even switch
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS6(STD_ROW5)), module, CVSpreader::MODE_PARAM));
		
		// base and spread input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, CVSpreader::BASE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, CVSpreader::SPREAD_INPUT));
		
		// base output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW1]), module, CVSpreader::F_OUTPUT)); // Base
		
		// Sum outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, CVSpreader::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, CVSpreader::B_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, CVSpreader::C_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, CVSpreader::D_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, CVSpreader::E_OUTPUT));
		
		// Diff outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, CVSpreader::G_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, CVSpreader::H_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, CVSpreader::I_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, CVSpreader::J_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, CVSpreader::K_OUTPUT));	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		CVSpreader *module = dynamic_cast<CVSpreader*>(this->module);
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

Model *modelCVSpreader = createModel<CVSpreader, CVSpreaderWidget>("CVSpreader");
