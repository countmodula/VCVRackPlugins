//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Polyphonic Voltage Controlled Switch Module
//	A polyphonic 2 in/1 out 1 in/2 out voltage controlled switch
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyVCSwitch
#define PANEL_FILE "PolyVCSwitch.svg"

struct PolyVCSwitch : Module {
	enum ParamIds {
		MANUAL_PARAM,
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
		ENUMS(SELECT_LIGHT, 32),
		MANUAL_PARAM_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gateSwitch[16];
	
	bool aConnected = false;
	bool bConnected = false;
	bool bUseCV;
	int nA, nB1, nB2, nB;
	int count = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	PolyVCSwitch() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configSwitch(MANUAL_PARAM, 0.0f, 1.0f, 0.0f, "Select", {"A (Red)", "B (Green)"});
	
		configInput(CV_INPUT, "Select CV");
		inputInfos[CV_INPUT]->description = "Disconnects the manual select button";
		configInput(A_INPUT, "Switch 1");
		configInput(B1_INPUT, "Switch 2 A");
		configInput(B2_INPUT, "Switch 2 B");
	
		configOutput(A1_OUTPUT, "Switch 1 A");
		configOutput(A2_OUTPUT, "Switch 1 B");
		configOutput(B_OUTPUT, "Switch 2");
	
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		for (int i = 0; i < 16; i ++) {
			gateSwitch[i].reset();
		}
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
		
		bUseCV = inputs[CV_INPUT].isConnected();
		float manual = clamp(params[MANUAL_PARAM].getValue(), 0.0f, 1.0f) * 10.0f;
		
		nA = inputs[A_INPUT].getChannels();
		nB1 = inputs[B1_INPUT].getChannels();
		nB2 = inputs[B2_INPUT].getChannels();
		nB = std::max(nB1, nB2);
		
		if (inputs[A_INPUT].isConnected()) {
			aConnected = true;
			outputs[A1_OUTPUT].setChannels(nA);
			outputs[A2_OUTPUT].setChannels(nA);
		}
		else {
			aConnected = false;
			outputs[A1_OUTPUT].channels = 0;
			outputs[A2_OUTPUT].channels = 0;
		}
		
		if (inputs[B1_INPUT].isConnected() && inputs[B2_INPUT].isConnected()) {
			bConnected = true;
			outputs[B_OUTPUT].setChannels(nB);
		}
		else {
			bConnected = false;
			outputs[B_OUTPUT].channels = 0;
		}
		
		for (int c = 0; c < 16; c++) {
			if (bUseCV)
				gateSwitch[c].set(inputs[CV_INPUT].getPolyVoltage(c));
			else
				gateSwitch[c].set(manual);
			
			if (gateSwitch[c].high()) {
				// IN A -> OUT A2
				if (aConnected && c < nA) {
					outputs[A1_OUTPUT].setVoltage(0.0f, c);
					outputs[A2_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
				}
				
				// IN B2 -> OUT B
				if (bConnected)
					outputs[B_OUTPUT].setVoltage(inputs[B2_INPUT].getPolyVoltage(c), c);
			}
			else {
				// IN A -> OUT A1
				if (aConnected && c < nA) {
					outputs[A1_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
					outputs[A2_OUTPUT].setVoltage(0.0f, c);
				}
				
				// IN B1 -> OUT B
				if (bConnected)
					outputs[B_OUTPUT].setVoltage(inputs[B1_INPUT].getPolyVoltage(c), c);
			}
			
			if (count == 0) {
				lights[SELECT_LIGHT + (c * 2)].setBrightness(boolToLight(gateSwitch[c].low()));
				lights[SELECT_LIGHT + (c * 2) + 1].setBrightness(boolToLight(gateSwitch[c].high()));
			}
		}
		
		if (++count > 3)
			count = 0;
	}
};


struct PolyVCSwitchWidget : ModuleWidget {

	std::string panelName;
	
	PolyVCSwitchWidget(PolyVCSwitch *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
	
		// cv input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS7[STD_ROW1]), module, PolyVCSwitch::CV_INPUT));

		// manual selection
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS7[STD_ROW2]), module, PolyVCSwitch::MANUAL_PARAM, PolyVCSwitch::MANUAL_PARAM_LIGHT));
	
		// 1 - 2
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 10, STD_ROWS7[STD_ROW5]), module, PolyVCSwitch::A_INPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 10, STD_ROWS7[STD_ROW6]), module, PolyVCSwitch::A1_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 10, STD_ROWS7[STD_ROW7]), module, PolyVCSwitch::A2_OUTPUT));
		
		// 2 - 1
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS7[STD_ROW5]), module, PolyVCSwitch::B_OUTPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS7[STD_ROW6]), module, PolyVCSwitch::B1_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS7[STD_ROW7]), module, PolyVCSwitch::B2_INPUT));

		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS7[STD_ROW3] - 5;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + (10 * x++), startPos + (10 * y)), module, PolyVCSwitch::SELECT_LIGHT + (s * 2)));
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyVCSwitch *module = dynamic_cast<PolyVCSwitch*>(this->module);
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

Model *modelPolyVCSwitch = createModel<PolyVCSwitch, PolyVCSwitchWidget>("PolyVCSwitch");
