//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Multi-step Sequencer Module
//	A multi-step CV/Gate sequencer
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME MultiStepSequencer
#define WIDGET_NAME MultiStepSequencerWidget
#define MODULE_NAME "MultiStepSequencer"
#define MODEL_NAME	modelMultiStepSequencer

#define SEQ_NUM_STEPS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MultiStepSequencer
#define PANEL_FILE "MultiStepSequencer.svg"

#define PROBABILITY_MODE_STP 1
#define PROBABILITY_MODE_RPT 2
#define PROBABILITY_MODE_OFF 0

struct SequenceEngine {
	const int maxCount = 40320;
	int id = 0;
	int repeatCount = 0;
	int clockCount = 1;
	bool state = false;
	bool gate = false;
	bool clock = false;
	float repeatCV = 0.0f;

	SequenceEngine() {
		reset();
	}
	
	// called once every clock edge from the main controller - returns true if at end of cycle
	bool step(bool clk, int div, int rpt, float prob, int probMode) {
		bool eoc = false;
		
		// handle the clock divisions first
		bool clockEdge = false;
		if (--clockCount < 1) {
			clockCount = div;
			clockEdge = true;
		}
		

		if (clockEdge) {
			clock = true;
			
			if (++repeatCount > rpt) {
				repeatCount = 0;
				eoc = true;
				state = gate = false;
			}
			else {
				gate = true;
				
				// apply probability if required
				switch (probMode) {
					case PROBABILITY_MODE_RPT:
						applyProbability(prob);
						break;
					case PROBABILITY_MODE_OFF:
						state = true;
						break;
				}
			}
			
			float d = std::fmax(1.0f, rpt);
			float c = std::fmax(1.0f, (float)(repeatCount));
			repeatCV = c/d * 10.0f;
		}

		return eoc;
	}
	
	void processClock(bool masterClock) {
		clock &= masterClock;
	}
	
	void reset() {
		repeatCount = 0;
		clockCount = 1;
		state = false;
	}
	
	void applyProbability(float prob) {
		float r = random::uniform();
		state = (r <= prob);
	}
	
};

struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_RPT_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_PRB_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_DIV_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_PRBMODE_PARAMS, SEQ_NUM_STEPS),
		LENGTH_PARAM,
		SAMPLEMODE_PARAM,
		RANGE_SW_PARAM,
		DIR_PARAM,
		ENUMS(STEP_ON_PARAMS, SEQ_NUM_STEPS),
		NUM_PARAMS
	};
	
	enum InputIds {
		RUN_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		LENCV_INPUT,
		STEP_INPUT, 
		DIRCV_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		TRIG_OUTPUT,
		GATE_OUTPUT,
		CV_OUTPUT,
		CVI_OUTPUT,
		STEP_CHANGE_OUTPUT,
		GATED_STEP_CHANGE_OUTPUT,
		DIV_OUTPUT,
		RPT_OUTPUT,
		EOC_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(DIR_LIGHTS, 3),
		TRIG_LIGHT,
		GATE_LIGHT,
		ENUMS(STEP_ON_LIGHTS, SEQ_NUM_STEPS),
		STEP_CHANGE_LIGHT,
		GATED_STEP_CHANGE_LIGHT,
		DIV_LIGHT,
		EOC_LIGHT,
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
	dsp::PulseGenerator pgEOC;
	
	SequenceEngine sequencers[SEQ_NUM_STEPS];
	
	int length = 8;
	int direction = 0;
	int directionMode = 0;
	bool running = false;
	bool prevTrig = false;
	float cv = 0.0f;
	float rptCV = 0.0f;
	dsp::PulseGenerator pgClock;
	
	float moduleVersion = 1.3f;
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);

	int seq = -1;
	bool seqChange = false;

	int probabilityMode = PROBABILITY_MODE_OFF;
	
	float lengthParamValue = (float)(SEQ_NUM_STEPS);
	int directionParamValue = 0;
	bool sampleHoldModeOff = true;
	float scale = 8.0f;
	
	
	int processCount = 12;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length param
		configParam(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Length");

		// direction mode param
		configSwitch (DIR_PARAM, 0.0f, 2.0, 0.0f, "Direction", {"Forward", "Pendulum", "Reverse"});
		
		// range control
		configSwitch(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale", {"1 Volt", "2 Volts", "3 Volts", "4 Volts", "5 Volts", "6 Volts", "7 Volts", "8 Volts"} );
		configSwitch(SAMPLEMODE_PARAM, 0.0f, 1.0f, 0.0f, "Sample & hold mode", {"Off", "On"});

		// step lights and switches
		for (int s = 0, sw = 1; s < SEQ_NUM_STEPS; s++, sw++) {
			configSwitch(STEP_DIV_PARAMS + s, 1.0, 8.0f, 1.0f, rack::string::f("Step %d clock division", sw), {"1", "2", "3", "4", "5", "6", "7", "8"});
			configSwitch(STEP_RPT_PARAMS + s, 1.0f, 8.0f, 1.0f, rack::string::f("Step %d mode", sw), {"Single", "Repeat 2x", "Repeat 3x", "Repeat 4x", "Repeat 5x", "repeat 6x", "Repeat 7x", "Repeat 8x"});
			configParam(STEP_PRB_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d probability", sw), "%", 0.0f, 100.0f, 0.0f);
			configSwitch(STEP_PRBMODE_PARAMS + s, 0.0f, 2.0f, 1.0f, rack::string::f("Step %d probability mode", sw), {"Step", "Off", "Repeats"});
			configParam(STEP_CV_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d CV", sw), " V", 0.0f, 8.0f, 0.0f);
			configSwitch(STEP_ON_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d on/off", sw), {"Off", "On"});
			
			sequencers[s].id = s+1;
		}

		configInput(RUN_INPUT,   "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENCV_INPUT, "Length CV");
		configInput(DIRCV_INPUT, "Direction CV");
		
		configOutput(DIV_OUTPUT, "Divided clock");
		configOutput(TRIG_OUTPUT, "Trigger");
		configOutput(STEP_CHANGE_OUTPUT, "Step change");
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(GATED_STEP_CHANGE_OUTPUT, "Gated step change");
		configOutput(RPT_OUTPUT, "Repeat CV");
		configOutput(CV_OUTPUT, "CV");
		configOutput(CVI_OUTPUT, "Inverted CV");
		configOutput(EOC_OUTPUT, "End of cycle");

		for (int i = 0 ; i < SEQ_NUM_STEPS; i ++)
			sequencers[i].reset();
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		processCount = 12;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(moduleVersion));
		
		json_object_set_new(root, "currentStep", json_integer(seq));
		json_object_set_new(root, "direction", json_integer(direction));
		json_object_set_new(root, "clockState", json_boolean(gateClock.high()));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));

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
		
		if (currentStep)
			seq = json_integer_value(currentStep);
		
		if (dir)
			direction = json_integer_value(dir);
	
		
		if (clk)
			gateClock.preset(json_boolean_value(clk));
	
		
		if (run)
			gateRun.preset(json_boolean_value(run));
		
		running = gateRun.high();

		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"	
	}

	void onReset() override {
		
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		pgClock.reset();
		pgEOC.reset();
		
		length = SEQ_NUM_STEPS;
		direction = FORWARD;
		directionMode = FORWARD;
		prevTrig = false;
		seq = -1;
		seqChange = false;
		probabilityMode = PROBABILITY_MODE_OFF;
		
		for (int i = 0 ; i < SEQ_NUM_STEPS; i ++)
			sequencers[i].reset();
		
		processCount = 12;
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
		switch (directionMode) {
			case REVERSE:
				lights[DIR_LIGHTS].setBrightness(1.0f); 		// red
				lights[DIR_LIGHTS + 1].setBrightness(0.0f); 	// yellow
				lights[DIR_LIGHTS + 2].setBrightness(0.0f); 	// green
				break;
			case PENDULUM:
				lights[DIR_LIGHTS].setBrightness(0.0f); 		// red
				lights[DIR_LIGHTS + 1].setBrightness(0.5f); 	// yellow
				lights[DIR_LIGHTS + 2].setBrightness(0.0f); 	// green
				break;
			case FORWARD:
				lights[DIR_LIGHTS].setBrightness(0.0f); 		// red
				lights[DIR_LIGHTS + 1].setBrightness(0.0f); 	// yellow
				lights[DIR_LIGHTS + 2].setBrightness(1.0f); 	// green
				break;
		}
	}
	
	int  calculateLength() {
		int calculatedLength = 0;
		
		for (int i = 0 ; i < length; i++) {
			int d = (int)(params[STEP_DIV_PARAMS + i].getValue());
			int r = (int)(params[STEP_RPT_PARAMS + i].getValue());
			calculatedLength += (d * r);
		}
		
		return calculatedLength;
	}
	
	void setProbabilityMode() {
		switch ((int)(params[STEP_PRBMODE_PARAMS + seq].getValue())) {
			case 0:
				probabilityMode = PROBABILITY_MODE_STP;
				break;
			case 2:
				probabilityMode = PROBABILITY_MODE_RPT;
				break;
			default:
				probabilityMode =PROBABILITY_MODE_OFF;
				break;
		}
	}
	
	void process(const ProcessArgs &args) override {

		// no point reading control values at audio rate
		if (++processCount > 12) {
			processCount = 0;
			
			scale = params[RANGE_SW_PARAM].getValue();
			sampleHoldModeOff = params[SAMPLEMODE_PARAM].getValue() < 0.5f;
			directionParamValue = (int)(params[DIR_PARAM].getValue());
			lengthParamValue = (int)(params[LENGTH_PARAM].getValue());
		}

		// reset input
		float f = inputs[RESET_INPUT].getVoltage();
		gateReset.set(f);
		
		// run input
		f = inputs[RUN_INPUT].getNormalVoltage(10.f);
		gateRun.set(f);

		// clock
		f = inputs[CLOCK_INPUT].getVoltage(); 
		gateClock.set(f);

		// sequence length - jack overrides knob
		if (inputs[LENCV_INPUT].isConnected()) {
			// scale the input such that 10V = step 16, 0V = step 1
			length = (int)(clamp(lengthCVScale/10.0f * inputs[LENCV_INPUT].getVoltage(), 0.0f, lengthCVScale)) + 1;
		}
		else {
			length = lengthParamValue;
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
			directionMode = directionParamValue;

		setDirectionLight();

		// now process the steps for each row as required
		int nextDir = recalcDirection();
		
		// switch non-pendulum mode right away
		if (directionMode != PENDULUM)
			direction = nextDir;
		
		if (gateReset.leadingEdge()) {
			
			// restart pendulum at forward stage
			if (directionMode== PENDULUM)
				nextDir = FORWARD;
			
			// reset the count
			seq = -1;

			direction = nextDir;
			
			for (int i = 0 ; i < SEQ_NUM_STEPS; i ++)
				sequencers[i].reset();
		}

		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		bool clockEdge = gateClock.leadingEdge();
		if (clockEdge)
			pgClock.trigger(1e-4f);
		else if (pgClock.process(args.sampleTime)) {
			// if within cooey of the clock edge, run or reset is treated as a clock edge.
			clockEdge = (gateRun.leadingEdge() || gateReset.leadingEdge());
		}
		
		if (gateRun.low())
			running = false;
			
		// advance count on positive clock edge or the run edge if it is close to the clock edge
		if (clockEdge && gateRun.high()) {
			
			// on first clock after reset, we must set the correct sequencer to use.
			if (seq < 0) {
				switch (direction) {
					case FORWARD:
						seq = 0;
						break;
					case REVERSE:
						seq = length-1;
						break;
				}
				
				seqChange = true;
			}

			// flag that we are now actually running
			running = true;
			
			// step the current sequencer and move to the next if necessary
			if (sequencers[seq].step(gateClock.high(), (int)(params[STEP_DIV_PARAMS + seq].getValue()), (int)(params[STEP_RPT_PARAMS + seq].getValue()), params[STEP_PRB_PARAMS + seq].getValue(), probabilityMode))
			{
				seqChange = true;
				sequencers[seq].reset();

				// if we need to shift the next sequencer (eoc on previous one or first time through)
				if (direction == FORWARD) {
					seq++;

					if (seq >= length) {
						if (nextDir == FORWARD) {
							seq = 0;
							// EOC only happens here for forward mode
							pgEOC.trigger(1e-3f);
						}
						else {
							// in pendulum mode we change direction here
							seq--;
							direction = nextDir;
						}
					}
				}
				else {
					seq--;
					
					if (seq < 0) {
						// EOC hapens here for reverse or pendulum mode
						pgEOC.trigger(1e-3f);
						
						if (nextDir == REVERSE)
							seq = length-1;
						else {
							// in pendulum mode we change direction here
							seq++;
							direction = nextDir;
						}
					}
				}
				
				if (seq > length - 1)
					seq = length - 1;
				
				setProbabilityMode();
				sequencers[seq].step(gateClock.high(), (int)(params[STEP_DIV_PARAMS + seq].getValue()),  (int)(params[STEP_RPT_PARAMS + seq].getValue()), params[STEP_PRB_PARAMS + seq].getValue(), probabilityMode);
				
				if (probabilityMode == PROBABILITY_MODE_STP) {
					sequencers[seq].applyProbability(params[STEP_PRB_PARAMS + seq].getValue());
				}
			}
			
			rptCV = sequencers[seq].repeatCV;
		}
		else if (seq >= 0) {
			sequencers[seq].processClock(gateClock.high());
		}

		// now process the lights and outputs
		bool trig = false, gate = false, divClock = false, gatedSeqChange = false;
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));
			
			if (c == seq) {
				gate = params[STEP_ON_PARAMS + c].getValue() > 0.5;
				lights[STEP_LIGHTS + c].setBrightness(1.0f);
			}
			else
				lights[STEP_LIGHTS + c].setBrightness(0.0f);
		}
		
		// determine gate/trig
		if (seq < 0) {
			cv = 0;
			rptCV = 0;
			gatedSeqChange = seqChange = false;
		}
		else {
			divClock = sequencers[seq].clock;
			gate &= running & sequencers[seq].state;
			trig = gate & sequencers[seq].state & divClock;
			seqChange &= divClock;
			gatedSeqChange = seqChange & gate;
			
			// grab CV values
			if (sampleHoldModeOff)
				cv = params[STEP_CV_PARAMS + seq].getValue() * scale;
			else if (trig && ! prevTrig)
				cv = params[STEP_CV_PARAMS + seq].getValue() * scale;
		}
		
		// set the outputs accordingly
		
		if (divClock) {
			outputs[DIV_OUTPUT].setVoltage(10.0f);
			lights[DIV_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[DIV_OUTPUT].setVoltage(0.0f);
			lights[DIV_LIGHT].setBrightness(0.0f);
		}
		
		if (seqChange) {
			outputs[STEP_CHANGE_OUTPUT].setVoltage(10.0f);
			lights[STEP_CHANGE_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[STEP_CHANGE_OUTPUT].setVoltage(0.0f);
			lights[STEP_CHANGE_LIGHT].setBrightness(0.0f);
		}
		
		if (trig) {
			outputs[TRIG_OUTPUT].setVoltage(10.0f);
			lights[TRIG_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[TRIG_OUTPUT].setVoltage(0.0f);
			lights[TRIG_LIGHT].setBrightness(0.0f);
		}
		
		if (gate) {
			outputs[GATE_OUTPUT].setVoltage(10.0f);
			lights[GATE_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[GATE_OUTPUT].setVoltage(0.0f);
			lights[GATE_LIGHT].setBrightness(0.0f);
		}
	
		if (gatedSeqChange) {
			outputs[GATED_STEP_CHANGE_OUTPUT].setVoltage(10.0f);
			lights[GATED_STEP_CHANGE_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[GATED_STEP_CHANGE_OUTPUT].setVoltage(0.0f);
			lights[GATED_STEP_CHANGE_LIGHT].setBrightness(0.0f);
		}
		
		if (pgEOC.remaining > 0.0f) {
			pgEOC.process(args.sampleTime);
			outputs[EOC_OUTPUT].setVoltage(10.0f);
			lights[EOC_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[EOC_OUTPUT].setVoltage(0.0f);
			lights[EOC_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		outputs[RPT_OUTPUT].setVoltage(rptCV);
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
		
		prevTrig = trig;
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	CountModulaLEDDisplayMedium *ledLength;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"

		// run
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::RUN_INPUT));

		// reset
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW4]), module, STRUCT_NAME::RESET_INPUT));
		
		// clock
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, STRUCT_NAME::CLOCK_INPUT));
				
		// length 
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME::LENCV_INPUT));
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME::LENGTH_PARAM));

		// direction
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW6]), module, STRUCT_NAME::DIR_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::DIRCV_INPUT));
		addChild(createLightCentered<MediumLight<CountModulaLightRYG>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 21, STD_HALF_ROWS8(STD_ROW5) - 5), module, STRUCT_NAME::DIR_LIGHTS));
		
		// CV 
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW7)), module, STRUCT_NAME::RANGE_SW_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW7)), module, STRUCT_NAME::SAMPLEMODE_PARAM));
		
		// step controls
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)] -15, STD_ROWS8[STD_ROW1] - 20.5), module, STRUCT_NAME::STEP_LIGHTS + s));
			addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)] + 15, STD_ROWS8[STD_ROW1] - 20.5), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME:: STEP_DIV_PARAMS + s));
			addParam(createParamCentered<RotarySwitch<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_ROWS8[STD_ROW3]), module, STRUCT_NAME:: STEP_RPT_PARAMS + s));
			addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_HALF_ROWS8(STD_ROW4)), module, STRUCT_NAME::STEP_PRB_PARAMS + s));
			addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::STEP_PRBMODE_PARAMS + s));
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::STEP_CV_PARAMS + s));
			addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5 + (s * 2)], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::STEP_ON_PARAMS + s, STRUCT_NAME::STEP_ON_LIGHTS + s));
		}

		int outCol = STD_COLUMN_POSITIONS[STD_COL5 + (SEQ_NUM_STEPS * 2)];
		int lightCol = outCol + 15;
		
		// output lights
		addChild(createLightCentered<MediumLight<BlueLight>>(Vec(lightCol, STD_ROWS8[STD_ROW1] - 19.5), module, STRUCT_NAME::STEP_CHANGE_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(lightCol , STD_ROWS8[STD_ROW2] - 19.5), module, STRUCT_NAME::DIV_LIGHT));
		addChild(createLightCentered<MediumLight<WhiteLight>>(Vec(lightCol, STD_ROWS8[STD_ROW5] - 19.5), module, STRUCT_NAME::GATED_STEP_CHANGE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(lightCol, STD_ROWS8[STD_ROW3] - 19.5), module, STRUCT_NAME::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(lightCol , STD_ROWS8[STD_ROW4] - 19.5), module, STRUCT_NAME::TRIG_LIGHT));
		
		addChild(createLightCentered<MediumLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1]+15, STD_ROWS8[STD_ROW6] - 19.5), module, STRUCT_NAME::EOC_LIGHT));
		
		// output jacks
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW1]), module, STRUCT_NAME::STEP_CHANGE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW2]), module, STRUCT_NAME::DIV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW3]), module, STRUCT_NAME::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW4]), module, STRUCT_NAME::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::GATED_STEP_CHANGE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW6]), module, STRUCT_NAME::RPT_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(outCol, STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::CVI_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, STRUCT_NAME::EOC_OUTPUT));
		
		// total length display
		ledLength = new CountModulaLEDDisplayMedium(3);
		ledLength->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW3] - 6));
		ledLength->setText(8);
		addChild(ledLength);
		
	}
	
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		bool triggerInit = true;
		bool cvInit = true;
		bool probInit = false;
		
		void onAction(const event::Action &e) override {
			// text for history menu item
			char buffer[100];
			if (!triggerInit && cvInit)
				sprintf(buffer, "initialize CV only");
			else if (triggerInit && !cvInit)
				sprintf(buffer, "initialize step selection only");
			else
				sprintf(buffer, "initialize CV and step selection only");
			
			if (probInit)
				sprintf(buffer, "initialize probability only");
				
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerInit) {
					widget->getParam(STRUCT_NAME::STEP_ON_PARAMS + c )->getParamQuantity()->reset();
				}
				
				// CV
				if (cvInit) {
					widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + c)->getParamQuantity()->reset();
				}
				
				// probability
				if (probInit) {
					widget->getParam(STRUCT_NAME::STEP_PRB_PARAMS + c)->getParamQuantity()->reset();
					widget->getParam(STRUCT_NAME::STEP_PRBMODE_PARAMS + c)->getParamQuantity()->reset();
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
	
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		bool triggerRand = true;
		bool cvRand = true;
		bool probRand = false;
		
		void onAction(const event::Action &e) override {
		
			// text for history menu item
			char buffer[100];
			if (!triggerRand && cvRand)
				sprintf(buffer, "randomize CV only");
			else if (triggerRand && !cvRand)
				sprintf(buffer, "randomize step selection only");
			else
				sprintf(buffer, "randomize CV and setep selection only");
			
			if (probRand)
				sprintf(buffer, "randomize probability only");
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = buffer;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerRand) {
					widget->getParam(STRUCT_NAME::STEP_ON_PARAMS + c)->getParamQuantity()->randomize();
				}
				
				// CV
				if (cvRand) {
					widget->getParam(STRUCT_NAME::STEP_CV_PARAMS + c)->getParamQuantity()->randomize();
				}
				// probability
				if (probRand) {
					widget->getParam(STRUCT_NAME::STEP_PRB_PARAMS + c)->getParamQuantity()->randomize();
					widget->getParam(STRUCT_NAME::STEP_PRBMODE_PARAMS + c)->getParamQuantity()->randomize();
				}
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
		
	struct InitMenu : MenuItem {
		WIDGET_NAME *widget;
	
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// CV and step selection only init
			InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>("CV and step selection only");
			initMenuItem->widget = widget;
			menu->addChild(initMenuItem);

			// CV only init
			InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("CV only");
			initCVMenuItem->widget = widget;
			initCVMenuItem->triggerInit = false;
			menu->addChild(initCVMenuItem);

			// trigger only init
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Step selection only");
			initTrigMenuItem->widget = widget;
			initTrigMenuItem->cvInit = false;
			menu->addChild(initTrigMenuItem);
			
			// probability only init
			InitMenuItem *initProbMenuItem = createMenuItem<InitMenuItem>("Probability only");
			initProbMenuItem->widget = widget;
			initProbMenuItem->triggerInit = false;
			initProbMenuItem->cvInit = false;
			initProbMenuItem->probInit = true;
			menu->addChild(initProbMenuItem);
			
			return menu;
		}
	};
	
	struct RandMenu : MenuItem {
		WIDGET_NAME *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// full channel random
			RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>("CV and step selection only");
			randMenuItem->widget = widget;
			menu->addChild(randMenuItem);

			// CV only random
			RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("CV only");
			randCVMenuItem->widget = widget;
			randCVMenuItem->triggerRand = false;
			menu->addChild(randCVMenuItem);

			// trigger only random
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Step selection only");
			randTrigMenuItem->widget = widget;
			randTrigMenuItem->cvRand = false;
			menu->addChild(randTrigMenuItem);
			
			// probability only random
			RandMenuItem *randProbMenuItem = createMenuItem<RandMenuItem>("Probability only");
			randProbMenuItem->widget = widget;
			randProbMenuItem->cvRand = false;
			randProbMenuItem->triggerRand = false;
			randProbMenuItem->probRand = true;
			menu->addChild(randProbMenuItem);
			
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
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));
		
		// initialize
		InitMenu *initMenuItem = createMenuItem<InitMenu>("Initialize", RIGHT_ARROW);
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// randomize
		RandMenu *randMenuItem = createMenuItem<RandMenu>("Randomize", RIGHT_ARROW);
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);		

	}
	
	void step() override {
		if (module) {
			STRUCT_NAME *m = dynamic_cast<STRUCT_NAME*>(this->module);
			int l = m->calculateLength();
			ledLength->setText(l);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
