//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerExpanderOut8
#define PANEL_FILE "SequencerExpanderOut8.svg"


struct SequencerExpanderOut8 : Module {

	enum ParamIds {
		MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		ENUMS(STEP_GATE_OUTPUTS, SEQ_NUM_STEPS),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(CHANNEL_LIGHTS, SEQUENCER_EXP_MAX_CHANNELS),
		NUM_LIGHTS
	};
	
	// Expander details
	int ExpanderID = SequencerExpanderMessage::OUT8;
	SequencerExpanderMessage leftMessages[2][1];	// messages from left module (master)
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerExpanderMessage *messagesFromMaster;
	
	int channelID = -1;
	int prevChannelID = -1;
	bool leftModuleAvailable = false; 
	
	// 0123
	// RGYB
	int colourMapDefault[4] = {0, 1, 2, 3};	// default colour map - matches the trigger sequencer colours RGYB
	int colourMapBinSeq[4] = {1, 2, 3, 0}; 	// colour map for binary sequencer- Puts the red knob last GYBR
	int colourMapSS[4] = {1, 3, 0, 2}; 		// colour map for step sequencer, matches the first row of knob colours 
	
	int *colourMap = colourMapDefault;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SequencerExpanderOut8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// mode switch
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");

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

		// details from master
		bool running = true;
		int count = 0;
		bool clock = false;
		int channelCounters[SEQUENCER_EXP_MAX_CHANNELS] = {0, 0, 0, 0};
		bool clockStates[SEQUENCER_EXP_MAX_CHANNELS] = {false, false, false, false};
		bool runningStates[SEQUENCER_EXP_MAX_CHANNELS] = {false, false, false, false};
		
		colourMap = colourMapDefault;
		
		// grab the detail from the left hand module if we have one
		leftModuleAvailable = false;
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
					
				leftModuleAvailable = true;
				messagesFromMaster = (SequencerExpanderMessage *)(leftExpander.consumerMessage);

				switch (messagesFromMaster->masterModule) {
					case SEQUENCER_EXP_MASTER_MODULE_BINARY:
						colourMap = colourMapBinSeq;
						break;
					case SEQUENCER_EXP_MASTER_MODULE_DUALSTEP:
						colourMap = colourMapSS;
						break;
					default:
						colourMap = colourMapDefault;
						break;
				}
				
				// grab the channel id for this expander type
				channelID = clamp(messagesFromMaster->channels[ExpanderID], -1, 3);

				// decode the counter array
				for(int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS; i++) {
					channelCounters[i] = messagesFromMaster->counters[i];
					clockStates[i] = messagesFromMaster->clockStates[i];
					runningStates[i] = messagesFromMaster->runningStates[i];
					
					 if (i == channelID) {
						count = std::max(channelCounters[i], 0);
						clock = clockStates[i];
						running = runningStates[i];
						
						// wrap counters > 8 back around to 1 for the sequencers
						if (messagesFromMaster->masterModule != SEQUENCER_EXP_MASTER_MODULE_GATEDCOMPARATOR) {
							while (count > SEQ_NUM_STEPS)
								count -= SEQ_NUM_STEPS;
						}
					}
				}
			}
		}
		else
			channelID = -1;
		
		// determine channel light colour
		int m = 4;
		switch (channelID)  {
			case 0:
			case 1:
			case 2:
			case 3:
				m = colourMap[channelID];
				break;
			default:
				m = 4; // always 4 (grey)
				break;
		}
		
		// now set the light colour
		for (int i = 0; i < 4; i ++)
			lights[CHANNEL_LIGHTS + i].setBrightness(boolToLight(i == m));

		// if in gate mode, the clock is irrelevant
		if (params[MODE_PARAM].getValue() < 0.5f)
			clock = true;
		
		// set step lights and outputs
		short bitMask = 0x01;
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			bool stepActive = false;
			
			// for gated comparator, we have multiple bits set at the same time
			if (leftModuleAvailable && messagesFromMaster->masterModule == SEQUENCER_EXP_MASTER_MODULE_GATEDCOMPARATOR) {
				stepActive = (((short)count & bitMask) == bitMask);
				
				// prepare for next bit
				bitMask = bitMask << 1;
			}
			else
				stepActive = ((c + 1) == count);
			
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			outputs[STEP_GATE_OUTPUTS + c].setVoltage(boolToGate(stepActive && clock && running));
		}

		// set up the detail for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// set next module's channel number
				if (channelID < 0) {
					// we have no left hand module
					messageToExpander->setDefaultValues();
				}
				else {
					// add the channel counters and gate states
					for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
						messageToExpander->counters[i] = channelCounters[i];
						messageToExpander->clockStates[i] = clockStates[i];
						messageToExpander->runningStates[i] = runningStates[i];
					}
					
					// pass through the other expander channel numbers and master module details
					if (messagesFromMaster) {
						for(int i = 0; i < SequencerExpanderMessage::NUM_EXPANDERS; i++) {
								messageToExpander->setChannel(messagesFromMaster->channels[i], i);
						}

						messageToExpander->masterModule = messagesFromMaster->masterModule;
					}
					
					// set the channel for the next expander of this type
					messageToExpander->setNextChannel(channelID, ExpanderID);
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct SequencerExpanderOut8Widget : ModuleWidget {

	std::string panelName;
	
	SequencerExpanderOut8Widget(SequencerExpanderOut8 *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and knobs
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderOut8::STEP_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderOut8::STEP_GATE_OUTPUTS + s));
		}

		// channel light
		addChild(createLightCentered<MediumLight<CountModulaLightRGYB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, SequencerExpanderOut8::CHANNEL_LIGHTS));
	
		// mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, SequencerExpanderOut8::MODE_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SequencerExpanderOut8 *module = dynamic_cast<SequencerExpanderOut8*>(this->module);
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

Model *modelSequencerExpanderOut8 = createModel<SequencerExpanderOut8, SequencerExpanderOut8Widget>("SequencerExpanderOut8");
