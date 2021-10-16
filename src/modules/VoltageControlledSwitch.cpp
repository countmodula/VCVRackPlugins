//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Switch Module
//	A 2 in/1 out 1 in/2 out voltage controlled switch
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME VoltageControlledSwitch
#define PANEL_FILE "VoltageControlledSwitch.svg"

struct VoltageControlledSwitch : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		A_INPUT,
		B1_INPUT,
		B2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A1_OUTPUT,
		A2_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A1_LIGHT,
		A2_LIGHT,
		B1_LIGHT,
		B2_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger stSwitch;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	VoltageControlledSwitch() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(CV_INPUT, "Select CV");
		inputInfos[CV_INPUT]->description = "Schmitt trigger input with thresholds at 3.6 and 3.2 volts.";
		
		configInput(A_INPUT, "Switch 1");
		configOutput(A1_OUTPUT, "Switch 1 A");
		configOutput(A2_OUTPUT, "Switch 1 B");
		
		configInput(B1_INPUT, "Switch 2 A");
		configInput(B2_INPUT, "Switch 2 B");
		configOutput(B_OUTPUT, "Switch 2");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		stSwitch.reset();
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
		
		float cv = inputs[CV_INPUT].getNormalVoltage(0.0f);
		
		// not using standard gate processor here due to different trigger levels
		stSwitch.process(rescale(cv, 3.2f, 3.6f, 0.0f, 1.0f));

		int nA = inputs[A_INPUT].getChannels();
		int nB1 = inputs[B1_INPUT].getChannels();
		int nB2 = inputs[B2_INPUT].getChannels();
		
		if (stSwitch.isHigh()) {
			// IN A -> OUT A2
			outputs[A1_OUTPUT].setChannels(nA);
			outputs[A2_OUTPUT].setChannels(nA);
			for (int c = 0; c < nA; c++) {
				outputs[A1_OUTPUT].setVoltage(0.0f, c);
				outputs[A2_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
			}
			
			// IN B2 -> OUT B
			outputs[B_OUTPUT].setChannels(nB2);
			for (int c = 0; c < nB2; c++)
				outputs[B_OUTPUT].setVoltage(inputs[B2_INPUT].getVoltage(c), c);
			
			lights[A1_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[A2_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[B1_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[B2_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		}
		else {
			// IN A -> OUT A1
			outputs[A1_OUTPUT].setChannels(nA);
			outputs[A2_OUTPUT].setChannels(nA);
			for (int c = 0; c < nA; c++) {
				outputs[A1_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
				outputs[A2_OUTPUT].setVoltage(0.0f, c);
			}
			
			// IN B1 -> OUT B
			outputs[B_OUTPUT].setChannels(nB1);
			for (int c = 0; c < nB2; c++)
				outputs[B_OUTPUT].setVoltage(inputs[B1_INPUT].getVoltage(c), c);
			
			lights[A1_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[A2_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[B1_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[B2_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
	}
};


struct VoltageControlledSwitchWidget : ModuleWidget {

	std::string panelName;
	
	VoltageControlledSwitchWidget(VoltageControlledSwitch *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, VoltageControlledSwitch::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, VoltageControlledSwitch::A_INPUT));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, VoltageControlledSwitch::B1_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, VoltageControlledSwitch::B2_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, VoltageControlledSwitch::A1_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, VoltageControlledSwitch::A2_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, VoltageControlledSwitch::B_OUTPUT));

		// lights
		int ledColumn = 2 + ((STD_COLUMN_POSITIONS[STD_COL1] + STD_COLUMN_POSITIONS[STD_COL2])/ 2);
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW2) + 4), module, VoltageControlledSwitch::A1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW3) + 4), module, VoltageControlledSwitch::A2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW4) + 5), module, VoltageControlledSwitch::B1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW5) + 5), module, VoltageControlledSwitch::B2_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		VoltageControlledSwitch *module = dynamic_cast<VoltageControlledSwitch*>(this->module);
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

Model *modelVoltageControlledSwitch = createModel<VoltageControlledSwitch, VoltageControlledSwitchWidget>("VoltageControlledSwitch");
