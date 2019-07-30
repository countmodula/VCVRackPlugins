//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

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
	
	// Expander
	SequencerExpanderMessage leftMessages[2][1];	// messages from left module (master)
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerExpanderMessage *messagesFromMaster;
	
	int channelID = -1;
	int prevChannelID = -1;
	
	// 0123
	// RGYB
	int colourMapDefault[4] = {0, 1, 2, 3};	// default colour map - matches the trigger sequencer colours RGYB
	int colourMapBinSeq[4] = {1, 2, 3, 0}; 	// colour map for binary sequencer- Puts the red knob last GYBR
	int colourMapSS[4] = {1, 3, 0, 2}; 		// colour map for step sequencer, matches the first row of knob colours 
	
	int *colourMap = colourMapDefault;
	
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
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
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
		if (leftExpander.module) {
			if (leftExpander.module->model == modelSequencerExpanderCV8 || leftExpander.module->model == modelSequencerExpanderOut8 || leftExpander.module->model == modelSequencerExpanderTrig8 ||
				leftExpander.module->model == modelTriggerSequencer8 || leftExpander.module->model == modelStepSequencer8 || leftExpander.module->model == modelBinarySequencer || 
				leftExpander.module->model == modelBasicSequencer8 || leftExpander.module->model == modelBurstGenerator) {
					
				messagesFromMaster = (SequencerExpanderMessage *)(leftExpander.consumerMessage);

				switch (messagesFromMaster->masterModule) {
					case SEQUENCER_EXP_MASTER_MODULE_BNRYSEQ:
						colourMap = colourMapBinSeq;
						break;
					case SEQUENCER_EXP_MASTER_MODULE_STEPSEQ:
						colourMap = colourMapSS;
						break;
					default:
					case SEQUENCER_EXP_MASTER_MODULE_DEFAULT:
						colourMap = colourMapDefault;
						break;
				}
				
				// grab the channel id
				channelID = clamp(messagesFromMaster->channelCV, -1, 3);

				// decode the counter array
				for(int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS; i++) {
					channelCounters[i] = messagesFromMaster->counters[i];
					clockStates[i] = messagesFromMaster->clockStates[i];
					runningStates[i] = messagesFromMaster->runningStates[i];
					
					if (i == channelID) {
						count = std::max(channelCounters[i], 0);
			
						// wrap counters > 8 back around to 1
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
	
		float cv = 0.0f;
		float scale = 1.0f;
		
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// set step lights here
			bool stepActive = ((c + 1) == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			
			// now determine the output values
			if (stepActive) {
				// determine which scale to use
				scale = getScale(params[RANGE_SW_PARAM].getValue());
				
				// now grab the cv value
				cv = params[STEP_CV_PARAMS + c].getValue() * scale;
			}
		}

		// set the outputs accordingly
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
		
		// set up the detail for any secondary expander
		if (rightExpander.module) {
			if (rightExpander.module->model == modelSequencerExpanderCV8 || rightExpander.module->model == modelSequencerExpanderOut8 || rightExpander.module->model == modelSequencerExpanderTrig8) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// set next module's channel number
				if (channelID < 0) {
					// we have no left hand module
					messageToExpander->setCVChannel(-1);
					messageToExpander->setTrigChannel(-1);
					messageToExpander->setOutChannel(-1);
					messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_DEFAULT;
				}
				else {
					messageToExpander->setNextCVChannel(channelID);
					
					// add the channel counters and gate states
					for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
						messageToExpander->counters[i] = channelCounters[i];
						messageToExpander->clockStates[i] = clockStates[i];
						messageToExpander->runningStates[i] = runningStates[i];
					}
					
					// pass through the trigger channel number and master module details
					if (messagesFromMaster) {
						messageToExpander->setTrigChannel(messagesFromMaster->channelTRIG);
						messageToExpander->setOutChannel(messagesFromMaster->channelOUT);
						messageToExpander->masterModule = messagesFromMaster->masterModule;
					}
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct SequencerExpanderCV8Widget : ModuleWidget {
	SequencerExpanderCV8Widget(SequencerExpanderCV8 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SequencerExpanderCV8.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// row lights and knobs
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderCV8::STEP_LIGHTS + s));
			addParam(createParamCentered<CountModulaKnobGrey>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, SequencerExpanderCV8::STEP_CV_PARAMS + s));
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
							
	void step() override {
		if (module) {
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
					((CountModulaKnob *)(p))->dirtyValue = -1;
				}
				
				((SequencerExpanderCV8*)module)->prevChannelID = cid;
			}
		}
		
		Widget::step();
	}	
	
};

Model *modelSequencerExpanderCV8 = createModel<SequencerExpanderCV8, SequencerExpanderCV8Widget>("SequencerExpanderCV8");
