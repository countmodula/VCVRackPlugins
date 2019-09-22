//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Polyrhythmic Generator Module
//	Generates polyrhythmns 
//  use of this module is not advised as it is no longer supported, use MkII instead
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/FrequencyDivider.hpp"

struct PolyrhythmicGenerator : Module {
	enum ParamIds {
		ENUMS(CV_PARAM, 8),
		ENUMS(DIV_PARAM, 8),
		ENUMS(MUTE_PARAM, 8),
		MUTEALL_PARAM,
		BEATMODE_PARAM,
		OUTPUTMODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CLOCK_INPUT, 8),
		ENUMS(RESET_INPUT, 8),
		ENUMS(CV_INPUT, 8),
		MUTEALL_INPUT,
		BEATMODE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUT, 8),
		POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(TRIG_LIGHT, 8),
		NUM_LIGHTS
	};

	FrequencyDividerOld dividers[8];
	dsp::PulseGenerator pgTriggers[8];
	GateProcessor gpResets[8];
	GateProcessor gpClocks[8];

	GateProcessor gpMuteAll;
	GateProcessor gpBeatMode;

	PolyrhythmicGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// division channels
		for (int i = 0; i < 8; i++) {
			// knobs
			configParam(CV_PARAM + i, -1.0f, 1.0f, 0.0f, "Division  CV amount", " %", 0.0f, 100.0f, 0.0f);
			configParam(DIV_PARAM + i, 0.0f, 10.0f, 0.0f, "Division");
			
			// buttons
			configParam(MUTE_PARAM + i, 0.0f, 1.0f, 0.0f, "Mute channel");
		
		}
		
		// global stuff
		configParam(OUTPUTMODE_PARAM, 0.0f, 3.0f, 0.0f, "Output mode");
		configParam(BEATMODE_PARAM, 0.0f, 1.0f, 1.0f, "Beat mode");
		configParam(MUTEALL_PARAM, 0.0f, 1.0f, 0.0f, "Global mute");
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
	}
	
	void onReset() override {
		for (int i = 0; i < 8; i++) {
			dividers[i].reset();
			pgTriggers[i].reset();
			gpResets[i].reset();
			gpClocks[i].reset();
		}
	}
	
	void process(const ProcessArgs &args) override {

		// determine global control values
		gpMuteAll.set(inputs[MUTEALL_INPUT].getVoltage());
		gpBeatMode.set(inputs[BEATMODE_INPUT].getVoltage());
		int countMode =  (gpBeatMode.high() || params[BEATMODE_PARAM].getValue() > 0.5f)? COUNT_DN : COUNT_UP;
		bool muteAll = gpMuteAll.high() || params[MUTEALL_PARAM].getValue() > 0.5f;
		
		float prevClock = 0.0f;
		bool prevPhase;
		float prevReset = 0.0f;
		float prevCV = 0.0f;
		
		outputs[POLY_OUTPUT].setChannels(8);
		for (int i = 0; i < 8; i++) {
			// set the required maximum division in the divider
			dividers[i].setMaxN(15);
			dividers[i].setCountMode(countMode);
			
			// handle the reset input
			float res = inputs[RESET_INPUT + i].getNormalVoltage(prevReset);
			gpResets[i].set(res);
			if(gpResets[i].leadingEdge()) {
				dividers[i].reset();
				pgTriggers[i].reset();
			}
			
			// save for rising edge determination
			prevPhase = dividers[i].phase;
			
			// calculate the current division value and set the division number
			float cv = inputs[CV_INPUT + i].getNormalVoltage(prevCV);
			float div = params[DIV_PARAM + i].getValue() + (params[CV_PARAM + i].getValue() * cv);
			dividers[i].setN(div);

			// clock the divider 
			float clock = inputs[CLOCK_INPUT + i].getNormalVoltage(prevClock);
			dividers[i].process(clock);
			gpClocks[i].set(clock);
			
			// fire off the trigger on a 0 to 1 rising edge
			if (!prevPhase &&  dividers[i].phase) {
				pgTriggers[i].trigger(1e-3f);
			}
			
			// determine the output value
			bool trigOut = false;
			if (!muteAll && params[MUTE_PARAM + i].getValue() < 0.5f ) {
				switch ((int)(params[OUTPUTMODE_PARAM].getValue())) {
					case 0: // trigger
						trigOut = pgTriggers[i].process(args.sampleTime);
						break;
					case 1: // gate
						trigOut = dividers[i].phase;
						break;
					case 2: // gated clock
						trigOut = gpClocks[i].high() &&  dividers[i].phase;
						break;
					case 3:	// clock
						trigOut = pgTriggers[i].process(args.sampleTime);
						if (gpClocks[i].low()) {
							trigOut = false;
							pgTriggers[i].reset();
						}
						else if (trigOut){
							// extend the pulse as long as the clock is high
							pgTriggers[i].trigger(1e-3f);
						}
						break;
				}
			}
			
			// output the trigger value to both the poly out and the individual channel then set the light
			outputs[POLY_OUTPUT].setVoltage(boolToGate(trigOut), i);
			outputs[TRIG_OUTPUT + i].setVoltage(boolToGate(trigOut));
			lights[TRIG_LIGHT + i].setSmoothBrightness(boolToLight(trigOut), args.sampleTime);
		
			// save these values for normalling in the next iteration 
			prevClock = clock;
			prevReset = res;
			prevCV = cv;
		}
	}
};


struct PolyrhythmicGeneratorWidget : ModuleWidget {
	PolyrhythmicGeneratorWidget(PolyrhythmicGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyrhythmicGenerator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// division channels
		for (int i = 0; i < 8; i++) {
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CLOCK_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::RESET_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CV_INPUT + i));

			// knobs
			addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CV_PARAM + i));
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL11], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::DIV_PARAM + i));
			
			// buttons
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL13]-10, STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::MUTE_PARAM + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL14], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::TRIG_LIGHT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL15], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::TRIG_OUTPUT + i));
		}
		
		// global stuff
		addParam(createParamCentered<CountModulaRotarySwitch5PosRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]-15), module, PolyrhythmicGenerator::OUTPUTMODE_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]-15), module, PolyrhythmicGenerator::BEATMODE_INPUT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]-15), module, PolyrhythmicGenerator::BEATMODE_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, PolyrhythmicGenerator::MUTEALL_INPUT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, PolyrhythmicGenerator::MUTEALL_PARAM));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, PolyrhythmicGenerator::POLY_OUTPUT));
	}
};

Model *modelPolyrhythmicGenerator = createModel<PolyrhythmicGenerator, PolyrhythmicGeneratorWidget>("PolyrhythmicGenerator");
