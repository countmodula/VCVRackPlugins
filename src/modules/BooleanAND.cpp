//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - AND Logic Gate Module
//	A 4 input AND gate logic module with inverted output
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BooleanAND
#define PANEL_FILE "BooleanAND.svg"

struct AndGate {
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

		// process the AND function
		isHigh = a.high() && b.high() && c.high() && d.high();
		
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

struct BooleanAND : Module {
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
		AND_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	AndGate gate[PORT_MAX_CHANNELS];
	Inverter inverter[PORT_MAX_CHANNELS];

	int numChans, bChannels, cChannels, dChannels, iChannels;
	float inA, inB, inC, inD;
	float out;
	bool iConnected;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BooleanAND() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(3));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
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
	
			// input A determines number of channels we'll work with
			numChans = inputs[A_INPUT].getChannels();
			bChannels = inputs[B_INPUT].getChannels();
			cChannels = inputs[C_INPUT].getChannels();
			dChannels = inputs[D_INPUT].getChannels();

			outputs[AND_OUTPUT].setChannels(numChans);
			outputs[INV_OUTPUT].setChannels(numChans);
			
			for (int c = 0; c < numChans; c++) {
				inA = inputs[A_INPUT].getPolyVoltage(c);
				inB = c < bChannels ? inputs[B_INPUT].getVoltage(c) : inA;
				inC = c < cChannels ? inputs[C_INPUT].getVoltage(c) : inB;
				inD = c < dChannels ? inputs[D_INPUT].getVoltage(c) : inC;
				
				//perform the logic
				out =  gate[c].process(inA, inB, inC, inD);
				outputs[AND_OUTPUT].setVoltage(out, c);
			
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
			outputs[AND_OUTPUT].setChannels(1);
			outputs[AND_OUTPUT].setVoltage(0.0f);
			
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
	}	
};

struct BooleanANDWidget : ModuleWidget {

	std::string panelName;
	
	BooleanANDWidget(BooleanAND *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]),module, BooleanAND::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]),module, BooleanAND::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]),module, BooleanAND::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]),module, BooleanAND::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]),module, BooleanAND::I_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, BooleanAND::AND_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, BooleanAND::INV_OUTPUT));
	}
	
		// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BooleanAND *module = dynamic_cast<BooleanAND*>(this->module);
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

Model *modelBooleanAND = createModel<BooleanAND, BooleanANDWidget>("BooleanAND");
