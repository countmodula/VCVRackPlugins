//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - VC NOT Logic Gate Module
//	A Voltage controlled logical inverter
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BooleanVCNOT
#define PANEL_FILE "BooleanVCNOT.svg"

struct BooleanVCNOT : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(LOGIC_INPUT, 2),
		ENUMS(ENABLE_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(INV_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Inverter inverter;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BooleanVCNOT() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(LOGIC_INPUT, "A");
		configInput(ENABLE_INPUT, "A Invert");
		configOutput(INV_OUTPUT, "Inverted/not inverted A");
		inputInfos[ENABLE_INPUT]->description = "Apply a high gate voltage to invert or a low gate voltage to not invert";

		configInput(LOGIC_INPUT + 1, "B");
		configInput(ENABLE_INPUT + 1, "B Invert");
		configOutput(INV_OUTPUT + 1, "Inverted/not inverted B");
		inputInfos[ENABLE_INPUT + 1]->description = "Apply a high gate voltage to invert or a low gate voltage to not invert";
		
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
		inverter.reset();
	}
	
	void process(const ProcessArgs &args) override {
		//perform the logic
		for (int i = 0; i < 2; i++) {
			
			if (inputs[LOGIC_INPUT + i].isConnected()) {
				int numChannels = inputs[LOGIC_INPUT + i].getChannels();
				int numInvChannels = inputs[ENABLE_INPUT +i].getChannels();
				
				outputs[INV_OUTPUT + i].setChannels(numChannels);
				bool enable = inputs[ENABLE_INPUT + i].isConnected();

				int e = 0;
				for (int c = 0; c < numChannels; c++) {
					float inv = 10.0f;
					
					if (enable && e < numInvChannels)
						inv = inputs[ENABLE_INPUT + i].getVoltage(e);
					
					if (numInvChannels > 1)
						e++;
					
					outputs[INV_OUTPUT + i].setVoltage(inverter.process(inputs[LOGIC_INPUT + i].getVoltage(c), inv), c);
				}
			}
			else
				outputs[INV_OUTPUT + i].channels = 0;
		}
	}	
	
};

struct BooleanVCNOTWidget : ModuleWidget {

	std::string panelName;
	
	BooleanVCNOTWidget(BooleanVCNOT *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"	

		for (int i = 0; i < 2; i++) {
			// logic and enable inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + (i * 3)]),module, BooleanVCNOT::LOGIC_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + (i * 3)]),module, BooleanVCNOT::ENABLE_INPUT + i));
		
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + (i * 3)]), module, BooleanVCNOT::INV_OUTPUT + i));
		}
	}
	
		// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BooleanVCNOT *module = dynamic_cast<BooleanVCNOT*>(this->module);
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

Model *modelBooleanVCNOT = createModel<BooleanVCNOT, BooleanVCNOTWidget>("BooleanVCNOT");
