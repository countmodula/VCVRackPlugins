//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Trigger sequencer gate expander
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerExpanderTSG
#define PANEL_FILE "SequencerExpanderTSG.svg"

struct SequencerExpanderTSG : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		ENUMS(GATE_OUTPUTS, SEQUENCER_EXP_NUM_TRIGGER_OUTS),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(GATE_LIGHTS, SEQUENCER_EXP_NUM_TRIGGER_OUTS),
		NUM_LIGHTS
	};
	
	// Expander details
	SequencerExpanderMessage leftMessages[2][1];	// messages from left module (master)
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerExpanderMessage *messagesFromMaster;
	
	bool leftModuleAvailable = false; 
	
	bool gateValues[SEQUENCER_EXP_NUM_TRIGGER_OUTS] = {};
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SequencerExpanderTSG() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char c = 'A';
		for (int i = 0; i < SEQUENCER_EXP_NUM_TRIGGER_OUTS; i++) {
			configOutput(GATE_OUTPUTS + i, rack::string::f("Gate %c", c++));
		}
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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

		for (int i = 0; i < SEQUENCER_EXP_NUM_TRIGGER_OUTS; i++)
			gateValues[i] = false;
	
		// grab the detail from the left hand module if we have one
		leftModuleAvailable = false;
		if (leftExpander.module) {
			if (leftExpander.module->model == modelTriggerSequencer8 || leftExpander.module->model == modelTriggerSequencer16) {
				leftModuleAvailable = true;
				messagesFromMaster = (SequencerExpanderMessage *)(leftExpander.consumerMessage);
				for (int i = 0; i < SEQUENCER_EXP_NUM_TRIGGER_OUTS; i++)
						gateValues[i] = messagesFromMaster->gateStates[i];
			}
		}

		// set the lights and outputs
		for (int i = 0; i < SEQUENCER_EXP_NUM_TRIGGER_OUTS; i++) {
			lights[GATE_LIGHTS + i].setBrightness(boolToLight(gateValues[i]));
			outputs[GATE_OUTPUTS + i].setVoltage(boolToGate(gateValues[i]));
		}
		
		// set up the detail for any secondary expander - don;t bother about the gate expander as there can be only one
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// just pass the message through - the gate expander does not use any of the other expander values
				if (leftModuleAvailable) {
					for(int i = 0; i < SequencerExpanderMessage::NUM_EXPANDERS; i++)
						messageToExpander->setChannel(messagesFromMaster->channels[i], i);
					
					for(int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS; i++) {
						messageToExpander->counters[i] = messagesFromMaster->counters[i];
						messageToExpander->clockStates[i] = messagesFromMaster->clockStates[i];
						messageToExpander->runningStates[i] = messagesFromMaster->runningStates[i];
					}
					
					messageToExpander->masterModule = messagesFromMaster->masterModule;
				}
				else {
					// we have no left hand module
					messageToExpander->setDefaultValues();
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct SequencerExpanderTSGWidget : ModuleWidget {

	std::string panelName;
	
	SequencerExpanderTSGWidget(SequencerExpanderTSG *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	

		// gate lights and knobs
		for (int s = 0; s < SEQUENCER_EXP_NUM_TRIGGER_OUTS; s++) {
			addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderTSG::GATE_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderTSG::GATE_OUTPUTS + s));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SequencerExpanderTSG *module = dynamic_cast<SequencerExpanderTSG*>(this->module);
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

Model *modelSequencerExpanderTSG = createModel<SequencerExpanderTSG, SequencerExpanderTSGWidget>("SequencerExpanderTSG");
