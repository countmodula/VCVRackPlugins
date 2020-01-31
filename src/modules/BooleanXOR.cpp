//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - XOR Logic Gate Module
//	A 4 input Exclusive-OR (XOR) gate logic module with inverted output
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BooleanXOR
#define PANEL_FILE "BooleanXOR.svg"

struct XorGate {
	GateProcessor a;
	GateProcessor b;
	GateProcessor c;
	GateProcessor d;

	bool isHigh = false;
	
	float process (float aIn, float bIn, float cIn, float dIn, bool onehot) {
		a.set(aIn);
		b.set(bIn);
		c.set(cIn);
		d.set(dIn);

		// process the XOR function
		int i = 0;
		if (a.high()) i++;
		if (b.high()) i++;
		if (c.high()) i++;
		if (d.high()) i++;
		
		if (onehot)
			isHigh = i == 1;
		else
			isHigh = (1 > 0 && isOdd(i));
		
		return boolToGate(isHigh);
	}
	
	void reset() {
		a.reset();
		b.reset();
		c.reset();
		d.reset();
		
		isHigh = false;
	}
};

struct BooleanXOR : Module {
	enum ParamIds {
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		I_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		XOR_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	XorGate gate;
	Inverter inverter;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BooleanXOR() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "One-hot mode");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(1.2f));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		json_t *ver = json_object_get(root, "moduleVersion");

		if (ver) {
			// for modules < v1.2, one-hot was the default so set that here
			if (json_number_value(ver) < 1.2f)
				params[MODE_PARAM].setValue(1.0f);
		}
	}	
	
	void onReset() override {
		gate.reset();
		inverter.reset();
	}
	
	void process(const ProcessArgs &args) override {
		// grab and normalise the inputs
		float inA = inputs[A_INPUT].getNormalVoltage(0.0f);
		float inB = inputs[B_INPUT].getNormalVoltage(0.0f);
		float inC = inputs[C_INPUT].getNormalVoltage(0.0f);
		float inD = inputs[D_INPUT].getNormalVoltage(0.0f);
		
		//perform the logic
		float out = gate.process(inA, inB, inC, inD, params[MODE_PARAM].getValue() > 0.5f);
		outputs[XOR_OUTPUT].setVoltage(out);
		
		float notOut = inverter.process(inputs[I_INPUT].getNormalVoltage(out));
		outputs[INV_OUTPUT].setVoltage(notOut);
	}
};

struct BooleanXORWidget : ModuleWidget {
	BooleanXORWidget(BooleanXOR *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BooleanXOR.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]),module, BooleanXOR::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]),module, BooleanXOR::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]),module, BooleanXOR::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]),module, BooleanXOR::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]),module, BooleanXOR::I_INPUT));
		
		addParam(createParamCentered<CountModulaPBSwitchMini>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, BooleanXOR::MODE_PARAM));
	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, BooleanXOR::XOR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, BooleanXOR::INV_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BooleanXOR *module = dynamic_cast<BooleanXOR*>(this->module);
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

Model *modelBooleanXOR = createModel<BooleanXOR, BooleanXORWidget>("BooleanXOR");
