//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - OR Logic Gate Module
//	A 4 input OR gate logic module with inverted output
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BooleanOR
#define PANEL_FILE "BooleanOR.svg"

struct OrGate {
	GateProcessor a;
	GateProcessor b;
	GateProcessor c;
	GateProcessor d;

	bool isHigh = false;
	
	float process (float aIn, float bIn, float cIn, float dIn) {
		a.set(aIn);
		b.set(bIn);
		c.set(cIn);
		d.set(dIn);

		// process the OR function
		isHigh = a.high()|| b.high() || c.high() || d.high();
		
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

struct BooleanOR : Module {
	enum ParamIds {
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
		OR_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	OrGate gate;
	Inverter inverter;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BooleanOR() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.1"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;
	}	
	
		
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void onReset() override {
		gate.reset();
		inverter.reset();
	}

	void process(const ProcessArgs &args) override {
		// grab and normalise the inputs
		float inA = inputs[A_INPUT].getNormalVoltage(0.0f);
		float inB = inputs[B_INPUT].getNormalVoltage(inA);
		float inC = inputs[C_INPUT].getNormalVoltage(inB);
		float inD = inputs[D_INPUT].getNormalVoltage(inC);
		
		//perform the logic
		float out =  gate.process(inA, inB, inC, inD);
		outputs[OR_OUTPUT].setVoltage(out);
		
		float notOut = inverter.process(inputs[I_INPUT].getNormalVoltage(out));
		outputs[INV_OUTPUT].setVoltage(notOut);
	}
	
};

struct BooleanORWidget : ModuleWidget {
	BooleanORWidget(BooleanOR *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BooleanOR.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]),module, BooleanOR::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]),module, BooleanOR::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]),module, BooleanOR::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]),module, BooleanOR::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]),module, BooleanOR::I_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, BooleanOR::OR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, BooleanOR::INV_OUTPUT));
	}
	
		// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BooleanOR *module = dynamic_cast<BooleanOR*>(this->module);
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

Model *modelBooleanOR = createModel<BooleanOR, BooleanORWidget>("BooleanOR");
