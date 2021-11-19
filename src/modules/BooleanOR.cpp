//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - OR Logic Gate Module
//	A 4 input OR gate logic module with inverted output
//	Copyright (C) 2019  Adam Verspaget
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
		
		// process the OR function
		isHigh = false;
		isHigh |= a.set(aIn);
		isHigh |= b.set(bIn);
		isHigh |= c.set(cIn);
		isHigh |= d.set(dIn);
		
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

	OrGate gate[PORT_MAX_CHANNELS];
	Inverter inverter[PORT_MAX_CHANNELS];

	int numChans, bChannels, cChannels, dChannels, iChannels;
	float inA, inB, inC, inD;
	float out;
	bool iConnected;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BooleanOR() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configInput(C_INPUT, "C");
		configInput(D_INPUT, "D");
		configInput(I_INPUT, "Inverter");
		
		inputInfos[B_INPUT]->description = "Normalled to A Input";
		inputInfos[C_INPUT]->description = "Normalled to B Input";
		inputInfos[D_INPUT]->description = "Normalled to C Input";
		inputInfos[I_INPUT]->description = "Normalled to OR output";

		configOutput(OR_OUTPUT, "Logical OR");
		configOutput(INV_OUTPUT, "Inverter");
		
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

			outputs[OR_OUTPUT].setChannels(numChans);
			outputs[INV_OUTPUT].setChannels(numChans);
			
			for (int c = 0; c < numChans; c++) {
				inA = inputs[A_INPUT].getPolyVoltage(c);
				inB = c < bChannels ? inputs[B_INPUT].getVoltage(c) : inA;
				inC = c < cChannels ? inputs[C_INPUT].getVoltage(c) : inB;
				inD = c < dChannels ? inputs[D_INPUT].getVoltage(c) : inC;
				
				//perform the logic
				out =  gate[c].process(inA, inB, inC, inD);
				outputs[OR_OUTPUT].setVoltage(out, c);
			
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
			outputs[OR_OUTPUT].setChannels(1);
			outputs[OR_OUTPUT].setVoltage(0.0f);
			
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

struct BooleanORWidget : ModuleWidget {

	std::string panelName;
	
	BooleanORWidget(BooleanOR *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

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
