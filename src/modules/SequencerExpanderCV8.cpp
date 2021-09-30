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
#define THEME_MODULE_NAME SequencerExpanderCV8
#define PANEL_FILE "SequencerExpanderCV8.svg"

struct SequencerExpanderCV8 : Module {

	enum ParamIds {
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS),
		RANGE_SW_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		CV_OUTPUT,
		CVI_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(CHANNEL_LIGHTS, SEQUENCER_EXP_MAX_CHANNELS),
		NUM_LIGHTS
	};
	
	// Expander details
	int ExpanderID = SequencerExpanderMessage::CV8;
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
	
	SequencerExpanderCV8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// step params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configParam(STEP_CV_PARAMS + s, 0.0f, 8.0f, 0.0f, "Step value");
		}
		
		// range switch
		configParam(RANGE_SW_PARAM, 0.0f, 2.0f, 0.0f, "Scale");

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
	
		// determine which scale to use
		float scale = getScale(params[RANGE_SW_PARAM].getValue());

		// now deal with the CV and lights
		float cv = 0.0f;
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			bool stepActive = ((c + 1) == count);
			
			// grab the cv value
			if (stepActive)
				cv = params[STEP_CV_PARAMS + c].getValue() * scale;

			// set step lights here
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
		}

		// set the outputs accordingly
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
		
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
					// set the channel for the next expander of this type
					messageToExpander->setNextChannel(channelID, ExpanderID);
					
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
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct SequencerExpanderCV8Widget : ModuleWidget {

	std::string panelName;
	
	SequencerExpanderCV8Widget(SequencerExpanderCV8 *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and knobs
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderCV8::STEP_LIGHTS + s));
			addParam(createParamCentered<Potentiometer<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderCV8::STEP_CV_PARAMS + s));
		}

		// channel light
		addChild(createLightCentered<MediumLight<CountModulaLightRGYB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, SequencerExpanderCV8::CHANNEL_LIGHTS));
		
		// range control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, SequencerExpanderCV8::RANGE_SW_PARAM));

		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, SequencerExpanderCV8::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, SequencerExpanderCV8::CVI_OUTPUT));
	}
	
	char knobColours[5][50] = {	"res/Components/KnobRed.svg", 
								"res/Components/KnobGreen.svg", 
								"res/Components/KnobYellow.svg",  
								"res/Components/KnobBlue.svg", 
								"res/Components/KnobGrey.svg"};   	
							
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"							
		
	void appendContextMenu(Menu *menu) override {
		SequencerExpanderCV8 *module = dynamic_cast<SequencerExpanderCV8*>(this->module);
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
		
			int cid = ((SequencerExpanderCV8*)module)->channelID;
			int pid = ((SequencerExpanderCV8*)module)->prevChannelID;
		
			if (cid != pid) {
				
				int m = 4;
				switch (cid) {
					case 0:
					case 1:
					case 2:
					case 3:
						m = ((SequencerExpanderCV8*)module)->colourMap[cid];
						break;
					default:
						m = 4; // always 4 (grey)
						break;
				}
	
				for (int i = 0; i < SEQ_NUM_STEPS; i++) {
					ParamWidget *p = getParam(SequencerExpanderCV8::STEP_CV_PARAMS + i);
					((CountModulaKnob *)(p))->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, knobColours[m]))); 
					//((CountModulaKnob *)(p))->dirtyValue = -1;
				}
				
				((SequencerExpanderCV8*)module)->prevChannelID = cid;
			}
		}
		
		Widget::step();
	}	
	
};

Model *modelSequencerExpanderCV8 = createModel<SequencerExpanderCV8, SequencerExpanderCV8Widget>("SequencerExpanderCV8");
