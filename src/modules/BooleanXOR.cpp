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
		int i = 0;
		i += (int)(a.set(aIn));
		i += (int)(b.set(bIn));
		i += (int)(c.set(cIn));
		i += (int)(d.set(dIn));

		// process the XOR function
		// int i = 0;
		// if (a.high()) i++;
		// if (b.high()) i++;
		// if (c.high()) i++;
		// if (d.high()) i++;
		
		if (onehot)
			isHigh = i == 1;
		else
			isHigh = (i > 0 && isOdd(i));
		
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

	XorGate gate[PORT_MAX_CHANNELS];
	Inverter inverter[PORT_MAX_CHANNELS];

	int numChans, bChannels, cChannels, dChannels, iChannels;
	float inA, inB, inC, inD;
	float out;
	bool iConnected, oneHot;
	
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

		json_object_set_new(root, "moduleVersion", json_real(1.3f));
		
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
		for (int i = 0; i < PORT_MAX_CHANNELS; i++) {
			gate[i].reset();
			inverter[i].reset();
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		iConnected  = inputs[I_INPUT].isConnected();
		
		if (inputs[A_INPUT].isConnected()) {

			// what mode are we in?
			oneHot = (params[MODE_PARAM].getValue() > 0.5f);
		
			// input A determines number of channels we'll work with
			numChans = inputs[A_INPUT].getChannels();
			bChannels = inputs[B_INPUT].getChannels();
			cChannels = inputs[C_INPUT].getChannels();
			dChannels = inputs[D_INPUT].getChannels();

			outputs[XOR_OUTPUT].setChannels(numChans);
			outputs[INV_OUTPUT].setChannels(numChans);
			
			for (int c = 0; c < numChans; c++) {
				inA = inputs[A_INPUT].getPolyVoltage(c);
				inB = c < bChannels ? inputs[B_INPUT].getVoltage(c) : 0.0f;
				inC = c < cChannels ? inputs[C_INPUT].getVoltage(c) : 0.0f;
				inD = c < dChannels ? inputs[D_INPUT].getVoltage(c) : 0.0f;
				
				//perform the logic
				out = gate[c].process(inA, inB, inC, inD, oneHot);
				outputs[XOR_OUTPUT].setVoltage(out, c);
			
				if (!iConnected)
					outputs[INV_OUTPUT].setVoltage(inverter[c].process(out), c);		
			}
	
			if (iConnected) {
				iChannels = inputs[I_INPUT].getChannels();
				outputs[INV_OUTPUT].setChannels(iChannels);
				for (int c = 0; c < iChannels; c++) {
					outputs[INV_OUTPUT].setVoltage(inverter[c].process(inputs[I_INPUT].getVoltage(c)), c);
				}
			}
		}
		else {
			outputs[XOR_OUTPUT].setChannels(1);
			outputs[XOR_OUTPUT].setVoltage(0.0f);
			
			if (iConnected) {
				iChannels = inputs[I_INPUT].getChannels();
				outputs[INV_OUTPUT].setChannels(iChannels);
				for (int c = 0; c < iChannels; c++) {
					outputs[INV_OUTPUT].setVoltage(inverter[c].process(inputs[I_INPUT].getVoltage(c)), c);
				}
			}
			else {
				outputs[INV_OUTPUT].setChannels(1);
				outputs[INV_OUTPUT].setVoltage(10.0f);
			}
		}
				
		// // grab and normalise the inputs
		// float inA = inputs[A_INPUT].getNormalVoltage(0.0f);
		// float inB = inputs[B_INPUT].getNormalVoltage(0.0f);
		// float inC = inputs[C_INPUT].getNormalVoltage(0.0f);
		// float inD = inputs[D_INPUT].getNormalVoltage(0.0f);
		
		// //perform the logic
		// float out = gate.process(inA, inB, inC, inD, params[MODE_PARAM].getValue() > 0.5f);
		// outputs[XOR_OUTPUT].setVoltage(out);
		
		// float notOut = inverter.process(inputs[I_INPUT].getNormalVoltage(out));
		// outputs[INV_OUTPUT].setVoltage(notOut);
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
