//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

struct BasicSequencer8 : Module {

	enum ParamIds {
		ENUMS(STEP_SW_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS),
		LENGTH_PARAM,
		CV_PARAM,
		RANGE_SW_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		RUN_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		LENCV_INPUT,
		DIRCV_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		TRIG_OUTPUT,
		GATE_OUTPUT,
		CV_OUTPUT,
		CVI_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		TRIG_LIGHT,
		GATE_LIGHT,
		ENUMS(DIR_LIGHTS, 3),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE
	};
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	
	int count = 0;
	int length = 8;
	int direction = 0;
	int directionMode = 0;
	
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif

	BasicSequencer8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length param
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");
		
		//  mode param
		configParam(MODE_PARAM, 0.0f, 2.0, 0.0f, "Direction");
		
		// step params (knobs and switches)
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configParam(STEP_SW_PARAMS + s, 0.0f, 2.0f, 1.0f, "Select Trig/Gate");

			configParam(STEP_CV_PARAMS + s, 0.0f, 8.0f, 0.0f, "Step value");
		}
		
		// range switch
		configParam(RANGE_SW_PARAM, 0.0f, 2.0f, 0.0f, "Scale");
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
#endif
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "direction", json_integer(direction));

		return root;
	}

	void dataFromJson(json_t *root) override {
		
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		
		if (currentStep)
			count = json_integer_value(currentStep);
		
		if (dir)
			direction = json_integer_value(dir);
	}

	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		count = 0;
		length = SEQ_NUM_STEPS;
		direction = FORWARD;
		directionMode = FORWARD;
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
	
	int recalcDirection() {
		
		switch (directionMode) {
			case PENDULUM:
				return (direction == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case REVERSE:
			default:
				return directionMode;
		}
	}
	
	void setDirectionLight() {
		lights[DIR_LIGHTS].setBrightness(boolToLight(directionMode == REVERSE)); 		// red
		lights[DIR_LIGHTS + 1].setBrightness(boolToLight(directionMode == PENDULUM) * 0.5f); 	// yellow
		lights[DIR_LIGHTS + 2].setBrightness(boolToLight(directionMode == FORWARD)); 	// green
	}
	
	void process(const ProcessArgs &args) override {

		// grab all the common input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;

		// reset input
		f = inputs[RESET_INPUT].getNormalVoltage(reset);
		gateReset.set(f);
		reset = f;
		
		// run input
		f = inputs[RUN_INPUT].getNormalVoltage(run);
		gateRun.set(f);
		run = f;

		// clock
		f = inputs[CLOCK_INPUT].getNormalVoltage(clock); 
		gateClock.set(f);
		clock = f;
		
		// sequence length - jack overrides knob
		if (inputs[LENCV_INPUT].isConnected()) {
			// scale the input such that 10V = step 16, 0V = step 1
			length = (int)(clamp(lengthCVScale/10.0f * inputs[LENCV_INPUT].getVoltage(), 0.0f, lengthCVScale)) + 1;
		}
		else {
			length = (int)(params[LENGTH_PARAM].getValue());
		}
		
		// set the length lights
		for(int i = 0; i < SEQ_NUM_STEPS; i++) {
			lights[LENGTH_LIGHTS +  i].setBrightness(boolToLight(i < length));
		}
			
		// direction - jack overrides the switch
		if (inputs[DIRCV_INPUT].isConnected()) {
			float dirCV = clamp(inputs[DIRCV_INPUT].getVoltage(), 0.0f, 10.0f);
			if (dirCV < 2.0f)
				directionMode = FORWARD;
			else if (dirCV < 4.0f)
				directionMode = PENDULUM;
			else
				directionMode = REVERSE;
		}
		else
			directionMode = (int)(params[MODE_PARAM].getValue());

		setDirectionLight();			
	
		// which mode are we using? jack overrides the switch
		int nextDir = recalcDirection();
		
		// switch non-pendulum mode right away
		if (directionMode != PENDULUM)
			direction = nextDir;
		
		if (gateReset.leadingEdge()) {
			
			// restart pendulum at forward stage
			if (directionMode == PENDULUM)
				direction = nextDir = FORWARD;
			
			// reset the count according to the next direction
			switch (nextDir) {
				case FORWARD:
					count = 0;
					break;
				case REVERSE:
					count = SEQ_NUM_STEPS;
					break;
			}
			
			direction = nextDir;
		}

		bool running = gateRun.high();
		if (running) {
			// advance count on positive clock edge
			if (gateClock.leadingEdge()){
				if (direction == FORWARD) {
					count++;
					
					if (count > length) {
						if (nextDir == FORWARD)
							count = 1;
						else {
							// in pendulum mode we change direction here
							count--;
							direction = nextDir;
						}
					}
				}
				else {
					count--;
					
					if (count < 1) {
						if (nextDir == REVERSE)
							count = length;
						else {
							// in pendulum mode we change direction here
							count++;
							direction = nextDir;
						}
					}
				}
			}
		}
		
		// now process the lights and outputs
		bool trig = false;
		bool gate = false;
		float cv = 0.0f;
		float scale = 1.0f;
		
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// set step lights here
			bool stepActive = ((c + 1) == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			
			// now determine the output values
			if (stepActive) {
				// trigger/gate
				switch ((int)(params[STEP_SW_PARAMS + c].getValue())) {
					case 0: // lower output
						trig = false;
						gate = true;
						break;
					case 2: // upper output
						trig = true;
						gate = false;
						break;				
					default: // off
						trig = false;
						gate = false;
						break;
				}
					
				// determine which scale to use
				scale = getScale(params[RANGE_SW_PARAM].getValue());
				
				// now grab the cv value
				cv = params[STEP_CV_PARAMS + c].getValue() * scale;
			}
		}

		// trig output follows clock width
		trig &= (running && gateClock.high());

		// gate output follows step widths
		gate &= running;

		// set the outputs accordingly
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig));	
		outputs[GATE_OUTPUT].setVoltage(boolToGate(gate));	
		
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
	
		lights[TRIG_LIGHT].setBrightness(boolToLight(trig));	
		lights[GATE_LIGHT].setBrightness(boolToLight(gate));
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (rightExpander.module->model == modelSequencerExpanderCV8 || rightExpander.module->model == modelSequencerExpanderOut8 || rightExpander.module->model == modelSequencerExpanderTrig8) {
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set the expander module's channel number
				messageToExpander->setCVChannel(0);
				messageToExpander->setTrigChannel(0);
				messageToExpander->setOutChannel(0);
	
				// add the channel counters and gates
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
					messageToExpander->counters[i] = count;
					messageToExpander->clockStates[i] =	gateClock.high();
					messageToExpander->runningStates[i] = running;
				}
				
				// finally, let all subsequent expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_DEFAULT;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif		
	}

};

struct BasicSequencer8Widget : ModuleWidget {
	BasicSequencer8Widget(BasicSequencer8 *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BasicSequencer8.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// run input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, BasicSequencer8::RUN_INPUT));

		// reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, BasicSequencer8::RESET_INPUT));
		
		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, BasicSequencer8::CLOCK_INPUT));
				
		// length CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, BasicSequencer8::LENCV_INPUT));

		// direction CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1]), module, BasicSequencer8::DIRCV_INPUT));
		
		// direction light
		addChild(createLightCentered<MediumLight<CountModulaLightRYG>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 5, STD_HALF_ROWS8(STD_ROW1) - 5), module, BasicSequencer8::DIR_LIGHTS));
			
		// length & mode params
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW2]), module, BasicSequencer8::MODE_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW3)), module, BasicSequencer8::LENGTH_PARAM));
			
		// row lights and switches
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] - 15, STD_ROWS8[STD_ROW1 + s] - 19), module, BasicSequencer8::LENGTH_LIGHTS + s));
			addParam(createParamCentered<CountModulaToggle3P90>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8:: STEP_SW_PARAMS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8::STEP_LIGHTS + s));
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8::STEP_CV_PARAMS + s));
		}
			
		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 20), module, BasicSequencer8::TRIG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8] - 20), module, BasicSequencer8::GATE_LIGHT));
			
		// range controls
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW5]), module, BasicSequencer8::RANGE_SW_PARAM));

		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW6)), module, BasicSequencer8::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW6)), module, BasicSequencer8::CVI_OUTPUT));
		
		// gate/trig output jack
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, BasicSequencer8::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8]), module, BasicSequencer8::GATE_OUTPUT));
	}
};

Model *modelBasicSequencer8 = createModel<BasicSequencer8, BasicSequencer8Widget>("BasicSequencer8");
