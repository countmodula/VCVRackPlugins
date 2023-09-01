//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//	A classic 8 step CV/Gate sequencer
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerExpanderTrig8
#define PANEL_FILE "SequencerExpanderTrig8.svg"

struct SequencerExpanderTrig8 : Module {

	enum ParamIds {
		ENUMS(STEP_SW_PARAMS, SEQ_NUM_STEPS),
		ENUMS(TRIGGER_PARAMS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAMS, SEQ_NUM_STEPS),
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		TRIG_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		TRIG_LIGHT,
		GATE_LIGHT,
		ENUMS(CHANNEL_LIGHTS, SEQUENCER_EXP_MAX_CHANNELS),
		ENUMS(TRIGGER_PARAM_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAM_LIGHTS, SEQ_NUM_STEPS),
		NUM_LIGHTS
	};
	
	// Expander details
	int ExpanderID = SequencerExpanderMessage::TRIG8;
	SequencerExpanderMessage leftMessages[2][1];	// messages from left module (master)
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerExpanderMessage *messagesFromMaster;
	
	int channelID = -1;
	int prevChannelID = -1;
	bool leftModuleAvailable = false; 
	
	int moduleVersion = 2;
	
	// 0123
	// RGYB
	int colourMapDefault[4] = {0, 1, 2, 3};	// default colour map - matches the trigger sequencer colours RGYB
	int colourMapBinSeq[4] = {1, 2, 3, 0}; 	// colour map for binary sequencer- Puts the red knob last GYBR
	int colourMapSS[4] = {1, 3, 0, 2}; 		// colour map for step sequencer, matches the first row of knob colours 
	
	int *colourMap = colourMapDefault;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SequencerExpanderTrig8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
		
		// step params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configSwitch(STEP_SW_PARAMS + s, 0.0f, 2.0f, 1.0f, "Select Trig/Gate", {"Gate", "Off", "Trigger"});
			configSwitch(TRIGGER_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d trigger select", s + 1), {"Off", "On"});
			configSwitch(GATE_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d gate select", s + 1), {"Off", "On"});
		}

		configOutput(TRIG_OUTPUT, "Trigger");
		configOutput(GATE_OUTPUT, "Gate");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(moduleVersion));
		
			// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		json_t *version = json_object_get(root, "moduleVersion");		
	
		if (version)
			moduleVersion = json_integer_value(version);
		else {
			// no version in file is really old version.
			moduleVersion = 0;
		}
	
		// conversion to new step select switches
		if (moduleVersion < 2) {
			INFO("Converting from module version %d", moduleVersion);

			for(int i = 0; i < SEQ_NUM_STEPS; i++) {
				int x = (int)(params[STEP_SW_PARAMS + i].getValue());

				params[GATE_PARAMS + i].setValue(x == 0 ? 1.0f : 0.0f);
				params[TRIGGER_PARAMS + i].setValue(x == 2 ? 1.0f : 0.0f);
			}
			
			moduleVersion = 2;
		}	
	}	
	
	void process(const ProcessArgs &args) override {

		// details from master
		bool running = true;
		bool clock = false;
		int count = 0;
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

						// wrap counters > 8 back around to 1 - for the gated comparator, we'll just treat the shift register like a counter
						while (count > SEQ_NUM_STEPS)
							count -= SEQ_NUM_STEPS;
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
	
		// now process the lights and outputs
		bool trig = false;
		bool gate = false;	
			
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			bool stepActive = ((c + 1) == count);

			// set step lights here
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			
			// now determine the output values
			if (stepActive) {
				
				if(params[TRIGGER_PARAMS + c].getValue() > 0.5f) {
					trig = true;
				}
				
				if(params[GATE_PARAMS + c].getValue() > 0.5f) {
					gate = true;
				}
				
				// // trigger/gate
				// switch ((int)(params[STEP_SW_PARAMS + c].getValue())) {
					// case 0: // lower output
						// trig = false;
						// gate = true;
						// break;
					// case 2: // upper output
						// trig = true;
						// gate = false;
						// break;				
					// default: // off
						// trig = false;
						// gate = false;
						// break;
				// }
			}
		}
			
		// trig output follows clock width
		trig &= (running && clock);

		// gate output follows step widths
		gate &= running;	
		
		// set the outputs accordingly
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig));	
		outputs[GATE_OUTPUT].setVoltage(boolToGate(gate));	
	
		lights[TRIG_LIGHT].setBrightness(boolToLight(trig));	
		lights[GATE_LIGHT].setBrightness(boolToLight(gate));

	
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
							if (i != ExpanderID)
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

struct SequencerExpanderTrig8Widget : ModuleWidget {

	std::string panelName;
	
	SequencerExpanderTrig8Widget(SequencerExpanderTrig8 *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and switches
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 25, STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderTrig8::STEP_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 5, STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderTrig8::TRIGGER_PARAMS + s, SequencerExpanderTrig8::TRIGGER_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 15, STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderTrig8::GATE_PARAMS + s, SequencerExpanderTrig8::GATE_PARAM_LIGHTS + s));
			
		}

		// channel light
		addChild(createLightCentered<MediumLight<CountModulaLightRGYB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, SequencerExpanderTrig8::CHANNEL_LIGHTS));
		
		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 20), module, SequencerExpanderTrig8::TRIG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 20), module, SequencerExpanderTrig8::GATE_LIGHT));

		// gate/trig outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, SequencerExpanderTrig8::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, SequencerExpanderTrig8::GATE_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SequencerExpanderTrig8 *module = dynamic_cast<SequencerExpanderTrig8*>(this->module);
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

Model *modelSequencerExpanderTrig8 = createModel<SequencerExpanderTrig8, SequencerExpanderTrig8Widget>("SequencerExpanderTrig8");
