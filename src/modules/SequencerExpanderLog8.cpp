//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Gated Comparator logic expander
//  Adds logically mixed outputs to the Gated Comparator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerExpanderLog8
#define PANEL_FILE "SequencerExpanderLog8.svg"

struct SequencerExpanderLog8 : Module {

	enum ParamIds {
		ENUMS(BIT_PARAMS, SEQ_NUM_STEPS),
		MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		AND_OUTPUT,
		OR_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		AND_LIGHT,
		OR_LIGHT,
		ENUMS(CHANNEL_LIGHTS, SEQUENCER_EXP_MAX_CHANNELS),
		NUM_LIGHTS
	};
	
	// Expander details
	int ExpanderID = SequencerExpanderMessage::LOG8;
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
	
	SequencerExpanderLog8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// bit params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configParam(BIT_PARAMS + s, 0.0f, 1.0f, 0.0f, "Bit mask");
		}
		
		// mode switch
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	float getScale(float range) {		
		switch ((int)(range)) {
			case 2:
				return 0.25f; // 2 volts
			case 1:
				return 0.5f; // 4 volts
			case 0:
			default:
				return 1.0f; // 8 volts
		}
	}

	void process(const ProcessArgs &args) override {

		// details from master
		bool running = true;
		bool clock = false;
		int shiftRegister = 0;
		int shiftRegisters[SEQUENCER_EXP_MAX_CHANNELS] = {0, 0, 0, 0};
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
					shiftRegisters[i] = messagesFromMaster->counters[i];
					clockStates[i] = messagesFromMaster->clockStates[i];
					runningStates[i] = messagesFromMaster->runningStates[i];
			
					if (i == channelID) {
						clock = clockStates[i];
						running = runningStates[i];

						if (messagesFromMaster->masterModule == SEQUENCER_EXP_MASTER_MODULE_GATEDCOMPARATOR || messagesFromMaster->masterModule == SEQUENCER_EXP_MASTER_MODULE_BINARY) {
							// shift register or full scale counter based master - can use the value we're given
							shiftRegister = std::max(shiftRegisters[i], 0) & 0xFF;
						}
						else {
							// for non shift register or 8/16 step based sources, we need to translate the counter into a shift register value
							int count = std::max(shiftRegisters[i], 0);
							
							while (count > SEQ_NUM_STEPS)
								count -= SEQ_NUM_STEPS;

							if (count > 0) {
								// adjust 1-8 down to 0-7
								count--;
							
								// now convert to shift register
								shiftRegister = 0x01 << count;
							}
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
	
	
		// now process the lights and outputs
		bool outAND = false;
		bool outOR = false;	

		// step through each bit and set a value based on the bits that match
		short bitMask = 0x01;
		short setBits = 0;
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
	
			bool stepActive = (((short)shiftRegister & bitMask) == bitMask);
			
			// set step lights here
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));

			// now convert the switches to logic
			if (params[BIT_PARAMS + c].getValue() > 0.05f)
				setBits = setBits | bitMask;

			// prepare for next bit
			bitMask = bitMask << 1;
		}
		
		// now process the results
		outAND = (setBits > 0 && (((short)shiftRegister & setBits) == setBits))  && clock && running;
		outOR = (((short)shiftRegister & setBits) != 0)  && clock && running;

		// set the outputs accordingly
		outputs[AND_OUTPUT].setVoltage(boolToGate(outAND));	
		outputs[OR_OUTPUT].setVoltage(boolToGate(outOR));	
		
		// and the lights too
		lights[AND_LIGHT].setBrightness(boolToLight(outAND));	
		lights[OR_LIGHT].setBrightness(boolToLight(outOR));
		
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
						messageToExpander->counters[i] = shiftRegisters[i];
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

struct SequencerExpanderLog8Widget : ModuleWidget {
	SequencerExpanderLog8Widget(SequencerExpanderLog8 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SequencerExpanderLog8.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// row lights and switches
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderLog8::STEP_LIGHTS + s));
			addParam(createParamCentered<CountModulaToggle2P90>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderLog8::BIT_PARAMS + s));
		}

		// mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, SequencerExpanderLog8::MODE_PARAM));
		
		// channel light
		addChild(createLightCentered<MediumLight<CountModulaLightRGYB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, SequencerExpanderLog8::CHANNEL_LIGHTS));
		
		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 20), module, SequencerExpanderLog8::AND_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 20), module, SequencerExpanderLog8::OR_LIGHT));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, SequencerExpanderLog8::AND_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, SequencerExpanderLog8::OR_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SequencerExpanderLog8 *module = dynamic_cast<SequencerExpanderLog8*>(this->module);
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

Model *modelSequencerExpanderLog8 = createModel<SequencerExpanderLog8, SequencerExpanderLog8Widget>("SequencerExpanderLOG8");
