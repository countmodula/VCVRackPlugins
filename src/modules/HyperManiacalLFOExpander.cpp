//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Hyper Maniacal LFO Expander Module 
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/HyperManiacalLFOExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME HyperManiacalLFOExpander
#define PANEL_FILE "HyperManiacalLFOExpander.svg"

using simd::float_4;

struct HyperManiacalLFOExpander : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SIN_OUTPUTS, 6),
		ENUMS(SAW_OUTPUTS, 6),
		ENUMS(TRI_OUTPUTS, 6),
		ENUMS(SQR_OUTPUTS, 6),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	enum OffsetModes {
		OFFSET_BIPOLAR,
		OFFSET_UNIPOLAR,
		OFFSET_FOLLOW,
		NUM_OFFSET_MODES
	};

	const float factor = 5.0f/1.2f;
	
	int offsetMode = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	// Expander details
	HyperManiacalLFOExpanderMessage leftMessages[2][1];	// messages from left module (master)
	HyperManiacalLFOExpanderMessage *messagesFromMaster;
	
	HyperManiacalLFOExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		std::string oscName;
		for (int i=0; i < 6; i++) {
			oscName = "Oscillator " + std::to_string(i + 1);
			configOutput(SIN_OUTPUTS + i, oscName + " sine");
			configOutput(SAW_OUTPUTS + i, oscName + " saw");
			configOutput(TRI_OUTPUTS + i, oscName + " triangle");
			configOutput(SQR_OUTPUTS + i, oscName + " square");
		}
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		offsetMode = OFFSET_BIPOLAR;
	}

	void onReset() override {
		offsetMode = OFFSET_BIPOLAR;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();
		
		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "offsetMode", json_integer(offsetMode));	
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		json_t *o = json_object_get(root, "offsetMode");
		
		if (o)
			offsetMode = json_integer_value(o);
	}
	
	void process(const ProcessArgs &args) override {

		if (leftExpander.module && isExpandableModule(leftExpander.module)) {
			messagesFromMaster = (HyperManiacalLFOExpanderMessage *)(leftExpander.consumerMessage);

			float offset;
			switch(offsetMode) {
				case OFFSET_UNIPOLAR:
					offset = messagesFromMaster->unipolar ? 0.0f : -1.2f;
					break;
				case OFFSET_FOLLOW:
					offset = 0.0f;
					break;
				default:
				case OFFSET_BIPOLAR:
					offset = messagesFromMaster->unipolar ? 1.2f : 0.0f;
					break;
			}
			
			for (int i = 0; i < 6; i++) {
				outputs[SIN_OUTPUTS + i].setVoltage((messagesFromMaster->sin[i] - offset) * factor);
				outputs[SAW_OUTPUTS + i].setVoltage((messagesFromMaster->saw[i] - offset) * factor);
				outputs[TRI_OUTPUTS + i].setVoltage((messagesFromMaster->tri[i] - offset) * factor);
				outputs[SQR_OUTPUTS + i].setVoltage((messagesFromMaster->sqr[i] - offset) * factor);
			}
		}
		else {
			for (int i = 0; i < 6; i++) {
				outputs[SIN_OUTPUTS + i].setVoltage(0.0f);
				outputs[SAW_OUTPUTS + i].setVoltage(0.0f);
				outputs[TRI_OUTPUTS + i].setVoltage(0.0f);
				outputs[SQR_OUTPUTS + i].setVoltage(0.0f);
			}
		}
	}
};

struct HyperManiacalLFOExpanderWidget : ModuleWidget {

	std::string panelName;
	
	HyperManiacalLFOExpanderWidget(HyperManiacalLFOExpander *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"	

		// outputs
		for (int i = 0; i < 6; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + i]), module, HyperManiacalLFOExpander::SIN_OUTPUTS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW1 + i]), module, HyperManiacalLFOExpander::SAW_OUTPUTS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + i]), module, HyperManiacalLFOExpander::TRI_OUTPUTS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW1 + i]), module, HyperManiacalLFOExpander::SQR_OUTPUTS + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"


	//---------------------------------------------------------------------------------------------
	// offset mode menu
	//---------------------------------------------------------------------------------------------
	// offset mode menu item
	struct OffsetModeMenuItem : MenuItem {
		HyperManiacalLFOExpander *module;
		int mode;

		void onAction(const event::Action &e) override {
			module->offsetMode = mode;
		}
	};
	
	// offset mode menu
	struct OffsetModeMenu : MenuItem {
		HyperManiacalLFOExpander *module;

		const char *menuLabels[HyperManiacalLFOExpander::NUM_OFFSET_MODES] = {"Bipolar", "Unipolar", "Follow LFO" };

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < HyperManiacalLFOExpander::NUM_OFFSET_MODES; i++) {
				OffsetModeMenuItem *modeMenuItem = createMenuItem<OffsetModeMenuItem>(menuLabels[i], CHECKMARK(module->offsetMode == i));
				modeMenuItem->module = module;
				modeMenuItem->mode = i;
				menu->addChild(modeMenuItem);
			}

			return menu;
		}
	};

	void appendContextMenu(Menu *menu) override {
		HyperManiacalLFOExpander *module = dynamic_cast<HyperManiacalLFOExpander*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		
		// add the output mode menu
		OffsetModeMenu *modeMenu = createMenuItem<OffsetModeMenu>("Offset mode", RIGHT_ARROW);
		modeMenu->module = module;
		menu->addChild(modeMenu);		
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelHyperManiacalLFOExpander = createModel<HyperManiacalLFOExpander, HyperManiacalLFOExpanderWidget>("HyperManiacalLFOExpander");
