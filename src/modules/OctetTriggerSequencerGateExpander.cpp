//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Octet Trigger Sequencer gate expander
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/OctetTriggerSequencerExpanderMessage.hpp"

#define STRUCT_NAME OctetTriggerSequencerGateExpander
#define WIDGET_NAME OctetTriggerSequencerGateExpanderWidget
#define MODULE_NAME "OctetTriggerSequencerGateExpander"
#define PANEL_FILE "OctetTriggerSequencerGateExpander.svg"
#define MODEL_NAME modelOctetTriggerSequencerGateExpander

// set the module name for the theme selection functions
#define THEME_MODULE_NAME OctetTriggerSequencerGateExpander

struct STRUCT_NAME : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(GATEA_OUTPUTS, 8),
		ENUMS(GATEB_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATEA_LIGHTS, 16),
		ENUMS(GATEB_LIGHTS, 16),
		NUM_LIGHTS
	};
	
	int count;
	int currentChannel = 0;
	int processCount = 0;

	bool gateA =false;
	bool gateB = false;
	int selectedPatternA = 0;
	int selectedPatternB = 0;
	int chainedPatternMode = 0;
	int patternA = 0;
	int patternB = 0;

	bool clockEdge = false;
	bool chained = false, playingChannelB = false;

	// Expander details
	OctetTriggerSequencerExpanderMessage leftMessages[2][1];	// messages from left module (master)
	OctetTriggerSequencerExpanderMessage rightMessages[2][1]; 	// messages to right module (expander)
	OctetTriggerSequencerExpanderMessage *messagesFromMaster;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	// count to bit mappping
	STEP_MAP;

	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int i = 0; i < 8; i ++) {
			configOutput(GATEA_OUTPUTS + i, rack::string::f("Channel A Step %d gate", i + 1));
			configOutput(GATEB_OUTPUTS + i, rack::string::f("Channel B Step %d gate", i + 1));
		}

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
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

		// grab the detail from the left hand module if we have one
		clockEdge = chained = playingChannelB = false;
		count = currentChannel = 0;
		messagesFromMaster = 0; 
		if (leftExpander.module) {
			if (isExpandableModule(leftExpander.module)) {
				messagesFromMaster = (OctetTriggerSequencerExpanderMessage *)(leftExpander.consumerMessage);

				count = messagesFromMaster->counter;
				clockEdge = messagesFromMaster->clockEdge;
				selectedPatternA = messagesFromMaster->selectedPatternA;
				selectedPatternB = messagesFromMaster->selectedPatternB;
				chained = messagesFromMaster->chained;
				playingChannelB = messagesFromMaster->playingChannelB;
				chainedPatternMode = messagesFromMaster->chainedPatternMode;
				processCount = messagesFromMaster->processCount;
				gateA = messagesFromMaster ->gateA;
				gateB = messagesFromMaster ->gateB;
				currentChannel = messagesFromMaster->channel;
			}
		}
		
		if (clockEdge) {
			patternA = selectedPatternA;
			patternB = selectedPatternB;

			if (chained) {
				if (playingChannelB)
					patternA = 0;
				else
					patternB = 0;
			}

			// set the outputs and lights
			int i = 0;

			for (int b = 1; b < 9; b++) {
				if (count == b) {
					if (patternA & stepMap[b]) {
						lights[GATEA_LIGHTS + i].setBrightness(1.0f);
						outputs[GATEA_OUTPUTS + i].setVoltage(10.0f);
					}
					else {
						lights[GATEA_LIGHTS + i].setBrightness(0.0f);
						outputs[GATEA_OUTPUTS + i].setVoltage(0.0f);
					}

					if (patternB & stepMap[b]) {
						lights[GATEB_LIGHTS + i].setBrightness(1.0f);
						outputs[GATEB_OUTPUTS + i].setVoltage(10.0f);
					}
					else {
						lights[GATEB_LIGHTS + i].setBrightness(0.0f);
						outputs[GATEB_OUTPUTS + i].setVoltage(0.0f);
					}
					
				}
				else {
					lights[GATEA_LIGHTS + i].setBrightness(0.0f);
					outputs[GATEA_OUTPUTS + i].setVoltage(0.0f);

					lights[GATEB_LIGHTS + i].setBrightness(0.0f);
					outputs[GATEB_OUTPUTS + i].setVoltage(0.0f);
				}
				
				i++;
			}
		}
		
		// finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				OctetTriggerSequencerExpanderMessage *messageToExpander = (OctetTriggerSequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				if (messagesFromMaster) {
					messageToExpander->set(count, clockEdge, selectedPatternA, selectedPatternB, currentChannel, messagesFromMaster->hasMaster, playingChannelB, chained, chainedPatternMode, processCount, gateA, gateB);
				}
				else {
					messageToExpander->initialise();
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// step pots and lights
		for (int s = 0; s < 8; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 11, STD_ROWS8[STD_ROW1 + s] - 19), module, STRUCT_NAME::GATEA_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME::GATEA_OUTPUTS + s));

			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 11, STD_ROWS8[STD_ROW1 + s] - 19), module, STRUCT_NAME::GATEB_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME::GATEB_OUTPUTS + s));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
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

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
