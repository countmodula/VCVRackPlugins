//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//  A classic 8 step CV/Gate sequencer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define STRUCT_NAME StepSequencer8
#define WIDGET_NAME StepSequencer8Widget
#define MODULE_NAME "StepSequencer8"
#define MODEL_NAME	modelStepSequencer8

#define SEQ_NUM_SEQS	2
#define SEQ_NUM_STEPS	8

#define SEQ_STEP_INPUTS

// set the module name for the theme selection functions
#define THEME_MODULE_NAME StepSequencer8
#define PANEL_FILE "StepSequencer8.svg"

struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_SW_PARAMS, SEQ_NUM_STEPS * 2 * SEQ_NUM_SEQS),
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS * 2 * SEQ_NUM_SEQS),
		ENUMS(LENGTH_PARAMS, SEQ_NUM_SEQS),
		ENUMS(CV_PARAMS, SEQ_NUM_SEQS),
		ENUMS(MUTE_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(RANGE_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(RANGE_SW_PARAMS, SEQ_NUM_SEQS * 2),
		ENUMS(MODE_PARAMS, SEQ_NUM_SEQS),
		NUM_PARAMS
	};
	
	enum InputIds {
		ENUMS(RUN_INPUTS, SEQ_NUM_SEQS),
		ENUMS(CLOCK_INPUTS, SEQ_NUM_SEQS),
		ENUMS(RESET_INPUTS, SEQ_NUM_SEQS),
		ENUMS(LENCV_INPUTS, SEQ_NUM_SEQS),
		ENUMS(STEP_INPUTS, SEQ_NUM_STEPS),
		ENUMS(DIRCV_INPUTS, SEQ_NUM_STEPS),
		NUM_INPUTS
	};
	
	enum OutputIds {
		ENUMS(TRIG_OUTPUTS, SEQ_NUM_SEQS * 2),
		ENUMS(GATE_OUTPUTS, SEQ_NUM_SEQS * 2),
		ENUMS(CV_OUTPUTS, 2 * SEQ_NUM_SEQS),
		ENUMS(CVI_OUTPUTS, 2 * SEQ_NUM_SEQS),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS * SEQ_NUM_SEQS),
		ENUMS(TRIG_LIGHTS, SEQ_NUM_SEQS * 2),
		ENUMS(GATE_LIGHTS, SEQ_NUM_SEQS * 2),
		ENUMS(DIR_LIGHTS, SEQ_NUM_SEQS * 3),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS * SEQ_NUM_SEQS),
		ENUMS(MUTE_PARAM_LIGHTS, SEQ_NUM_SEQS * 2),		
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE
	};
	
	GateProcessor gateClock[SEQ_NUM_SEQS];
	GateProcessor gateReset[SEQ_NUM_SEQS];
	GateProcessor gateRun[SEQ_NUM_SEQS];
	
	int count[SEQ_NUM_SEQS] = {}; 
	int length[SEQ_NUM_SEQS] = {};
	int direction[SEQ_NUM_SEQS] = {};
	int directionMode[SEQ_NUM_SEQS] = {};
	bool running[SEQ_NUM_SEQS] = {};
	dsp::PulseGenerator pgClock[SEQ_NUM_SEQS * 2];
	
	float moduleVersion = 1.3f;
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	float stepInputVoltages[SEQ_NUM_STEPS] = {};
	
	int startUpCounter = 0;
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char textBuffer[100];
		
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// length params
			sprintf(textBuffer, "Channel %d length", r + 1);
			configParam(LENGTH_PARAMS + r, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), textBuffer);
			
			//  mode params
			sprintf(textBuffer, "Channel %d direction", r + 1);
			configParam(MODE_PARAMS + r, 0.0f, 2.0, 0.0f, textBuffer);
			
			// row lights and switches
			int sw = 0;
			int k = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				sprintf(textBuffer, "Select %s", r == 0 ? "Trig A/ Trig B" : "Trig E/ Trig F");
				configParam(STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++, 0.0f, 2.0f, 1.0f, textBuffer);
				
				sprintf(textBuffer, "Select %s", r == 0 ? "Trig C/ Gate D" : "Trig G/ get H");
				configParam(STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++, 0.0f, 2.0f, 1.0f, textBuffer);

				sprintf(textBuffer, "Step value %s", r == 0 ? "(CV1/2)" : " (CV3)");
				configParam(STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++, 0.0f, 8.0f, 0.0f, textBuffer);
			}

			// add second row of knobs to channel 2
			if (r >  0) {
				for (int s = 0; s < SEQ_NUM_STEPS; s++) {
					configParam(STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++, 0.0f, 8.0f, 0.0f, "Step value (CV4)");
				}
			}
			
			// mute buttons
			char muteText[4][20] = {"Mute Trig A/Trig C",
									"Mute Trig B/Gate D",
									"Mute Trig E/Trig G",
									"Mute Trig F/Gate H"};
								
			for (int i = 0; i < 2; i++) {
				configParam(MUTE_PARAMS + (r * 2) + i, 0.0f, 1.0f, 0.0f, muteText[i + (r * 2)]);
			}
			
			// range controls
			for (int i = 0; i < 2; i++) {
				
				// use a single pot on the 1st row of the channel 1 and a switch on channel 2
				if (r == 0) {
					if (i == 0)
						configParam(RANGE_PARAMS + (r * 2) + i, 0.0f, 1.0f, 1.0f, "Range", " %", 0.0f, 100.0f, 0.0f);
				}
				else
					configParam(RANGE_SW_PARAMS + (r * 2) + i, 0.0f, 2.0f, 0.0f, "Scale");
			}			
		}
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
#endif	

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(moduleVersion));
		
		json_t *currentStep = json_array();
		json_t *dir = json_array();
		json_t *clk = json_array();
		json_t *run = json_array();
	
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			json_array_insert_new(currentStep, i, json_integer(count[i]));
			json_array_insert_new(dir, i, json_integer(direction[i]));
			json_array_insert_new(clk, i, json_boolean(gateClock[i].high()));
			json_array_insert_new(run, i, json_boolean(gateRun[i].high()));
		}
		
		json_object_set_new(root, "currentStep", currentStep);
		json_object_set_new(root, "direction", dir);
		json_object_set_new(root, "clockState", clk);
		json_object_set_new(root, "runState", run);
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t *root) override {
		
		json_t *version = json_object_get(root, "moduleVersion");
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "runState");
		
		if (version)
			moduleVersion = json_number_value(version);				
		
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			if (currentStep) {
				json_t *v = json_array_get(currentStep, i);
				if (v)
					count[i] = json_integer_value(v);
			}
			
			if (dir) {
				json_t *v = json_array_get(dir, i);
				if (v)
					direction[i] = json_integer_value(v);
			}
			
			if (clk) {
				json_t *v = json_array_get(clk, i);
				if (v)
					gateClock[i].preset(json_boolean_value(v));
			}
			
			if (run) {
				json_t *v = json_array_get(run, i);
				if (v)
					gateRun[i].preset(json_boolean_value(v));
				
				running[i] = gateRun[i].high();	
			}
		}
		
		// older module version, use the old length CV scale
		if (moduleVersion < 1.3f)
			lengthCVScale = (float)(SEQ_NUM_STEPS - 1);		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"	

		startUpCounter = 20;		
	}

	void onReset() override {
		
		for (int i = 0; i < SEQ_NUM_SEQS; i++) {
			gateClock[i].reset();
			gateReset[i].reset();
			gateRun[i].reset();
			pgClock[i].reset();
			count[i] = 0;
			length[i] = SEQ_NUM_STEPS;
			direction[i] = FORWARD;
			directionMode[i] = FORWARD;
		}
	}

	void onRandomize() override {
		// don't randomize mute buttons
		for (int i = 0; i < SEQ_NUM_SEQS * 2; i++) {
			params[MUTE_PARAMS + i].setValue(0.0f);
		}
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
	
	int recalcDirection(int r) {
		
		switch (directionMode[r]) {
			case PENDULUM:
				return (direction[r] == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case REVERSE:
			default:
				return directionMode[r];
		}
	}
	
	void setDirectionLight(int r) {
		lights[DIR_LIGHTS + (r * 3)].setBrightness(boolToLight(directionMode[r] == REVERSE)); 		// red
		lights[DIR_LIGHTS + (r * 3) + 1].setBrightness(boolToLight(directionMode[r] == PENDULUM) * 0.5f); 	// yellow
		lights[DIR_LIGHTS + (r * 3) + 2].setBrightness(boolToLight(directionMode[r] == FORWARD)); 	// green
	}
	
	void process(const ProcessArgs &args) override {

		// wait a number of cycles before we use the clock and run inputs to allow them propagate correctly after startup
		if (startUpCounter > 0)
			startUpCounter--;
		
		// grab all the common input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			// reset input
			f = inputs[RESET_INPUTS + r].getNormalVoltage(reset);
			gateReset[r].set(f);
			reset = f;
			
			if (startUpCounter == 0) {
				// run input
				f = inputs[RUN_INPUTS + r].getNormalVoltage(run);
				gateRun[r].set(f);
				run = f;

				// clock
				f = inputs[CLOCK_INPUTS + r].getNormalVoltage(clock); 
				gateClock[r].set(f);
				clock = f;
			}
			
			// sequence length - jack overrides knob
			if (inputs[LENCV_INPUTS + r].isConnected()) {
				// scale the input such that 10V = step 16, 0V = step 1
				length[r] = (int)(clamp(lengthCVScale/10.0f * inputs[LENCV_INPUTS + r].getVoltage(), 0.0f, lengthCVScale)) + 1;
			}
			else {
				length[r] = (int)(params[LENGTH_PARAMS + r].getValue());
			}
			
			// set the length lights
			for(int i = 0; i < SEQ_NUM_STEPS; i++) {
				lights[LENGTH_LIGHTS + (r * SEQ_NUM_STEPS) + i].setBrightness(boolToLight(i < length[r]));
			}
			
			// direction - jack overrides the switch
			if (inputs[DIRCV_INPUTS + r].isConnected()) {
				float dirCV = clamp(inputs[DIRCV_INPUTS + r].getVoltage(), 0.0f, 10.0f);
				if (dirCV < 2.0f)
					directionMode[r] = FORWARD;
				else if (dirCV < 4.0f)
					directionMode[r] = PENDULUM;
				else
					directionMode[r] = REVERSE;
			}
			else
				directionMode[r] = (int)(params[MODE_PARAMS + r].getValue());

			setDirectionLight(r);			
		}
		
		// pre-process the step inputs to determine what values to use
		// if input one is a polyphonic connection, we spread the channels across any unused inputs.
		int numChannels = inputs[STEP_INPUTS].getChannels();
		bool isPoly = numChannels > 1; 
		int j = 0;
		for (int i = 0; i < SEQ_NUM_STEPS; i ++) {
			if (i > 0 && inputs[STEP_INPUTS + i].isConnected())
				stepInputVoltages[i] = inputs[STEP_INPUTS + i].getVoltage();
			else {
				if (isPoly) {
					stepInputVoltages[i] = inputs[STEP_INPUTS].getVoltage(j++);
					if (j >= numChannels)
						j = 0;
				}
				else 
					stepInputVoltages[i] = inputs[STEP_INPUTS + i].getNormalVoltage(8.0f);
			}
		}

		// now process the steps for each row as required
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// which mode are we using? jack overrides the switch
			int nextDir = recalcDirection(r);
			
			// switch non-pendulum mode right away
			if (directionMode[r] != PENDULUM)
				direction[r] = nextDir;
			
			if (gateReset[r].leadingEdge()) {
				
				// restart pendulum at forward stage
				if (directionMode[r] == PENDULUM)
					direction[r]  = nextDir = FORWARD;
				
				// reset the count according to the next direction
				switch (nextDir) {
					case FORWARD:
						count[r] = 0;
						break;
					case REVERSE:
						count[r] = SEQ_NUM_STEPS + 1;
						break;
				}
				
				direction[r] = nextDir;
			}

			// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
			bool clockEdge = gateClock[r].leadingEdge();
			if (clockEdge)
				pgClock[r].trigger(1e-4f);
			else if (pgClock[r].process(args.sampleTime)) {
				// if within cooey of the clock edge, run or reset is treated as a clock edge.
				clockEdge = (gateRun[r].leadingEdge() || gateReset[r].leadingEdge());
			}
			
			if (gateRun[r].low())
				running[r] = false;
			
			// advance count on positive clock edge or the run edge if it is close to the clock edge
			if (clockEdge && gateRun[r].high()) {
				
				// flag that we are now actually running
				running[r] = true;
				
				if (direction[r] == FORWARD) {
					count[r]++;
					
					if (count[r] > length[r]) {
						if (nextDir == FORWARD)
							count[r] = 1;
						else {
							// in pendulum mode we change direction here
							count[r]--;
							direction[r] = nextDir;
						}
					}
				}
				else {
					count[r]--;
					
					if (count[r] < 1) {
						if (nextDir == REVERSE)
							count[r] = length[r];
						else {
							// in pendulum mode we change direction here
							count[r]++;
							direction[r] = nextDir;
						}
					}
				}
				
				if (count[r] > length[r])
					count[r] = length[r];
			}
	
			// now process the lights and outputs
			bool trig1 = false, trig2 = false;
			bool gate1 = false, gate2 = false;
			float cv1 = 0.0f, cv2 = 0.0f;
			float scale1 = 1.0f, scale2 = 1.0f;
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// set step lights here
				bool stepActive = (c + 1 == count[r]);
				lights[STEP_LIGHTS + (r * SEQ_NUM_STEPS) + c].setBrightness(boolToLight(stepActive));
				
				// now determine the output values
				if (stepActive) {
					// triggers
					switch ((int)(params[STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + (c * 2)].getValue())) {
						case 0: // lower output
							trig1 = false;
							trig2 = true;
							break;
						case 2: // upper output
							trig1 = true;
							trig2 = false;
							break;				
						default: // off
							trig1 = false;
							trig2 = false;
							break;
					}
					
					// gates
					switch ((int)(params[STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + (c * 2) +1].getValue())) {
						case 0: // lower output
							gate1 = false;
							gate2 = true;
							break;
						case 2: // upper output
							gate1 = true;
							gate2 = false;
							break;				
						default: // off
							gate1 = false;
							gate2 = false;
							break;
					}
					
					// determine which scale to use
					if (r == 0) {
						scale1 = scale2 = params[RANGE_PARAMS + (r * 2)].getValue();
					}
					else {
						scale1 = getScale(params[RANGE_SW_PARAMS + (r * 2)].getValue());
						scale2 = getScale(params[RANGE_SW_PARAMS + (r * 2) + 1].getValue());
					}
					
					// now grab the cv for row 1
					cv1 = params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + c].getValue() * scale1;
					
					if (r == 0) {
						// row 2 on channel 1 is the CV/audio inputs attenuated by the knobs
						cv2 = stepInputVoltages[c] * (params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + c].getValue()/ 8.0f) * scale2;
					}
					else {
						cv2 = params[STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + SEQ_NUM_STEPS + c].getValue() * scale2;
					}
				}
			}

			// trig outputs follow clock width
			trig1 &= (running[r] && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			trig2 &= (running[r] && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));

			// gate2 output follows step widths, gate 1 follows clock width
			gate1 &= (running[r] && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			gate2 &= (running[r] && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));

			// set the outputs accordingly
			outputs[TRIG_OUTPUTS + (r * 2)].setVoltage(boolToGate(trig1));	
			outputs[TRIG_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(trig2));

			outputs[GATE_OUTPUTS + (r * 2)].setVoltage(boolToGate(gate1));	
			outputs[GATE_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(gate2));
			
			outputs[CV_OUTPUTS + (r * 2)].setVoltage(cv1);
			outputs[CV_OUTPUTS + (r * 2) + 1].setVoltage(cv2);

			outputs[CVI_OUTPUTS + (r * 2)].setVoltage(-cv1);
			outputs[CVI_OUTPUTS + (r * 2) + 1].setVoltage(-cv2);
			
			lights[TRIG_LIGHTS + (r * 2)].setBrightness(boolToLight(trig1));	
			lights[TRIG_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(trig2));
			lights[GATE_LIGHTS + (r * 2)].setBrightness(boolToLight(gate1));	
			lights[GATE_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(gate2));
		}
				
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set any potential expander module's channel number
				messageToExpander->setAllChannels(0);
	
				// add the channel counters
				int j = 0;
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS; i++) {
					messageToExpander->counters[i] = count[j];
					messageToExpander->clockStates[i] =	gateClock[j].high();
					messageToExpander->runningStates[i] = running[j];
						
					// in case we ever add less than the expected number of rows, wrap them around to fill the expected buffer size
					if (++j == SEQ_NUM_SEQS)
						j = 0;
				}
					
				// finally, let all potential expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_DUALSTEP;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif		
	}

};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// add everything row by row
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			// run input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (r * 4)]), module, STRUCT_NAME::RUN_INPUTS + r));

			// reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::RESET_INPUTS + r));
			
			// clock input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::CLOCK_INPUTS + r));
					
			// length CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::LENCV_INPUTS + r));

			// direction CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW1 + (r * 4)]), module, STRUCT_NAME::DIRCV_INPUTS + r));
			
			// direction light
			addChild(createLightCentered<MediumLight<CountModulaLightRYG>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] - 1, STD_HALF_ROWS8(STD_ROW1 + (r * 4)) - 5), module, STRUCT_NAME::DIR_LIGHTS + (r * 3)));
			
			
			// length & mode params
			switch (r % SEQ_NUM_SEQS) {
				case 0:
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::MODE_PARAMS + r));
					addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_HALF_ROWS8(STD_ROW3 + (r * 4))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 1:
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_ROWS8[STD_ROW2 + (r * 4)]), module, STRUCT_NAME::MODE_PARAMS + r));
					addParam(createParamCentered<RotarySwitch<OrangeKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 20, STD_HALF_ROWS8(STD_ROW3 + (r * 4))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
			}
			
			// row lights and switches
			int i = 0;
			int sw = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW2 + (r * 4)] + 10), module, STRUCT_NAME::STEP_LIGHTS + (r * SEQ_NUM_STEPS) + i));
				addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)], STD_ROWS8[STD_ROW1 + (r * 4)] - 13), module, STRUCT_NAME::LENGTH_LIGHTS + (r * SEQ_NUM_STEPS) + i++));

				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)], STD_HALF_ROWS8(STD_ROW1 + (r * 4))), module, STRUCT_NAME:: STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++));
				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_HALF_ROWS8(STD_ROW1 + (r * 4))), module, STRUCT_NAME:: STEP_SW_PARAMS + (r * SEQ_NUM_STEPS * 2) + sw++));
			}

			// CV knob row 1
			int k = 0;
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				switch (r % SEQ_NUM_SEQS) {
					case 0:
						addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
					break;
				case 1:
						addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
					break;
				}
			}
			
			// CV knob row 2 (step inputs on 1st sequencer)
			for (int s = 0; s < SEQ_NUM_STEPS; s++) {
				switch (r % SEQ_NUM_SEQS) {
					case 0:
						addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::STEP_INPUTS + s));
						break;
					case 1:
						addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (s * 2)] + 15, STD_ROWS8[STD_ROW4 + (r * 4)]), module, STRUCT_NAME::STEP_CV_PARAMS + (r * SEQ_NUM_STEPS) + k++));
						break;
				}
			}
			
			// output lights, mute buttons
			for (int i = 0; i < 2; i++) {
				addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::MUTE_PARAMS + (r * 2) + i, STRUCT_NAME::MUTE_PARAM_LIGHTS + (r * 2) + i));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 1] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i] - 10), module, STRUCT_NAME::TRIG_LIGHTS + (r * 2) + i));
				addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 1] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i] + 10), module, STRUCT_NAME::GATE_LIGHTS + (r * 2) + i));
			}
			
			// range controls
			for (int i = 0; i < 2; i++) {
				
				// use a single pot on the 1st row of the channel 1 and a switch on channel 2
				if (r == 0) {
					if (i == 0)
						addParam(createParamCentered<Potentiometer<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_HALF_ROWS8(STD_ROW3 + (r * 4) + i)), module, STRUCT_NAME::RANGE_PARAMS + (r * 2) + i));
				}
				else
					addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2)] + 15, STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::RANGE_SW_PARAMS + (r * 2) + i));
			}
			
			// trig output jacks TA/TB
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 2] + 15, STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::TRIG_OUTPUTS + (r * 2) + i));
			
			// gate output jacks GC/GD
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + (SEQ_NUM_STEPS * 2) + 2], STD_ROWS8[STD_ROW1 + (r * 4) + i]), module, STRUCT_NAME::GATE_OUTPUTS + (r * 2) + i));

			// cv output jacks A/B
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4 + (SEQ_NUM_STEPS * 2) + 2] + 15, STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::CV_OUTPUTS + (r * 2) + i));

			// cv output jacks IA/IB
			for (int i = 0; i < 2; i++)
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + (SEQ_NUM_STEPS * 2) + 2], STD_ROWS8[STD_ROW3 + (r * 4) + i]), module, STRUCT_NAME::CVI_OUTPUTS + (r * 2) + i));
		}
	}
	
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {
			// text for history menu item
			char buffer[100];
			if (!triggerInit && cvInit)
				sprintf(buffer, "initialize channel %d CV", channel + 1);
			else if (triggerInit && !cvInit)
				sprintf(buffer, "initialize channel %d triggers", channel + 1);
			else
				sprintf(buffer, "initialize channel %d", channel + 1);
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// both trig and cv indicate we're doing the entire channel so do the common controls here
			if (triggerInit && cvInit) {
				// length switch
				widget->getParam(STRUCT_NAME::LENGTH_PARAMS + channel)->getParamQuantity()->reset();
				
				// direction
				widget->getParam(STRUCT_NAME::MODE_PARAMS + channel)->getParamQuantity()->reset();
				
				// scale/range/mutes
				for (int i = 0; i < 2; i++) {
					// use a single pot on the 1st row of the channel 1 and a switch on channel 2
					if (channel == 0) {
						if (i == 0)
							widget->getParam(STRUCT_NAME::RANGE_PARAMS + (channel * 2) + i)->getParamQuantity()->reset();
					}
					else
						widget->getParam(STRUCT_NAME::RANGE_SW_PARAMS + (channel * 2) + i)->getParamQuantity()->reset();
					
					// mutes
					widget->getParam(STRUCT_NAME::MUTE_PARAMS + (channel * 2) + i)->getParamQuantity()->reset();
				}
			}
			
			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerInit) {
					widget->getParam(STRUCT_NAME::STEP_SW_PARAMS + (channel * SEQ_NUM_STEPS * 2) + (c * 2))->getParamQuantity()->reset();
					widget->getParam(STRUCT_NAME::STEP_SW_PARAMS + (channel * SEQ_NUM_STEPS * 2) + (c * 2) +1)->getParamQuantity()->reset();
				}
				
				if (cvInit) {
					// cv row 1
					widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + (channel * SEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
					
					// cv row 2 (ch 2 only)
					if (channel > 0) 
						widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + (channel * SEQ_NUM_STEPS) + SEQ_NUM_STEPS + c)->getParamQuantity()->reset();
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	

		}
	};	
	
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		bool triggerRand = true;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
		
			// text for history menu item
			char buffer[100];
			if (!triggerRand && cvRand)
				sprintf(buffer, "randomize channel %d CV", channel + 1);
			else if (triggerRand && !cvRand)
				sprintf(buffer, "randomize channel %d triggers", channel + 1);
			else
				sprintf(buffer, "randomize channel %d", channel + 1);
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// both trig and cv indicate we're doing the entire channel so do the common controls here
			if (triggerRand && cvRand) {
				// length switch
				widget->getParam(STRUCT_NAME::LENGTH_PARAMS + channel)->getParamQuantity()->randomize();
				
				// direction
				widget->getParam(STRUCT_NAME::MODE_PARAMS + channel)->getParamQuantity()->randomize();
				
				// scale/range
				for (int i = 0; i < 2; i++) {
					// use a single pot on the 1st row of the channel 1 and a switch on channel 2
					if (channel == 0) {
						if (i == 0)
							widget->getParam(STRUCT_NAME::RANGE_PARAMS + (channel * 2) + i)->getParamQuantity()->randomize();
					}
					else
						widget->getParam(STRUCT_NAME::RANGE_SW_PARAMS + (channel * 2) + i)->getParamQuantity()->randomize();
				}
			}
			
			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerRand) {
					widget->getParam(STRUCT_NAME::STEP_SW_PARAMS + (channel * SEQ_NUM_STEPS * 2) + (c * 2))->getParamQuantity()->randomize();
					widget->getParam(STRUCT_NAME::STEP_SW_PARAMS + (channel * SEQ_NUM_STEPS * 2) + (c * 2) +1)->getParamQuantity()->randomize();
				}
				
				if (cvRand) {
					// cv row 1
					widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + (channel * SEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
					
					// cv row 2 (ch 2 only)
					if (channel > 0) 
						widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + (channel * SEQ_NUM_STEPS) + SEQ_NUM_STEPS + c)->getParamQuantity()->randomize();
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
		
	struct ChannelInitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
	
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// full channel init
			InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>("Entire Channel");
			initMenuItem->channel = channel;
			initMenuItem->widget = widget;
			menu->addChild(initMenuItem);

			// CV only init
			InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("CV Only");
			initCVMenuItem->channel = channel;
			initCVMenuItem->widget = widget;
			initCVMenuItem->triggerInit = false;
			menu->addChild(initCVMenuItem);

			// trigger only init
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Gates/Triggers Only");
			initTrigMenuItem->channel = channel;
			initTrigMenuItem->widget = widget;
			initTrigMenuItem->cvInit = false;
			menu->addChild(initTrigMenuItem);
			
			return menu;
		}
	
	};
	
	struct ChannelRandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// full channel random
			RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>("Entire Channel");
			randMenuItem->channel = channel;
			randMenuItem->widget = widget;
			menu->addChild(randMenuItem);

			// CV only random
			RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("CV Only");
			randCVMenuItem->channel = channel;
			randCVMenuItem->widget = widget;
			randCVMenuItem->triggerRand = false;
			menu->addChild(randCVMenuItem);

			// trigger only random
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Gates/Triggers Only");
			randTrigMenuItem->channel = channel;
			randTrigMenuItem->widget = widget;
			randTrigMenuItem->cvRand = false;
			menu->addChild(randTrigMenuItem);
			
			return menu;
		}	
	};
	
	struct ChannelMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// initialize
			ChannelInitMenuItem *initMenuItem = createMenuItem<ChannelInitMenuItem>("Initialize", RIGHT_ARROW);
			initMenuItem->channel = channel;
			initMenuItem->widget = widget;
			menu->addChild(initMenuItem);

			// randomize
			ChannelRandMenuItem *randMenuItem = createMenuItem<ChannelRandMenuItem>("Randomize", RIGHT_ARROW);
			randMenuItem->channel = channel;
			randMenuItem->widget = widget;
			menu->addChild(randMenuItem);

			return menu;
		}
	};
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"

		char textBuffer[100];
		for (int r = 0; r < SEQ_NUM_SEQS; r++) {
			
			sprintf(textBuffer, "Channel %d", r + 1);
			ChannelMenuItem *chMenuItem = createMenuItem<ChannelMenuItem>(textBuffer, RIGHT_ARROW);
			chMenuItem->channel = r;
			chMenuItem->widget = this;
			menu->addChild(chMenuItem);
		}
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
