//----------------------------------------------------------------------------
//	Count Modula - Voltage Controlled Divider Module
//	A voltage controlled frequency divider (divide by 1 - approx 20)
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
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(TRIG_LIGHT, 8),
		NUM_LIGHTS
	};

	FrequencyDivider dividers[8];
	PulseGenerator pgTriggers[8];
	GateProcessor gpResets[8];
	GateProcessor gpClocks[8];

	GateProcessor gpMuteAll;
	GateProcessor gpBeatMode;

	PolyrhythmicGenerator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	void onReset() override {
		for (int i = 0; i < 8; i++) {
			dividers[i].reset();
			pgTriggers[i].reset();
			gpResets[i].reset();
			gpClocks[i].reset();
		}
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void PolyrhythmicGenerator::step() {

	// determine global control values
	gpMuteAll.set(inputs[MUTEALL_INPUT].value);
	gpBeatMode.set(inputs[BEATMODE_INPUT].value);
	int countMode =  (gpBeatMode.high() || params[BEATMODE_PARAM].value > 0.5f)? COUNT_DN : COUNT_UP;
	bool muteAll = gpMuteAll.high() || params[MUTEALL_PARAM].value > 0.5f;
	
	float prevClock = 0.0f;
	bool prevPhase;
	float prevReset = 0.0f;
	float prevCV = 0.0f;
	
	for (int i = 0; i < 8; i++) {
		// set the required maximum division in the divider
		dividers[i].setMaxN(15);
		dividers[i].setCountMode(countMode);
		
		// handle the reset input
		float res = inputs[RESET_INPUT + i].normalize(prevReset);
		gpResets[i].set(res);
		if(gpResets[i].leadingEdge()) {
			dividers[i].reset();
			pgTriggers[i].reset();
		}
		
		// save for rising edge determination
		prevPhase = dividers[i].phase;
		
		// calculate the current division value and set the division number
		float cv = inputs[CV_INPUT + i].normalize(prevCV);
		float div = params[DIV_PARAM + i].value + (params[CV_PARAM + i].value * cv);
		dividers[i].setN(div);
		
		// clock the divider 
		float clock = inputs[CLOCK_INPUT + i].normalize(prevClock);
		dividers[i].process(clock);
		gpClocks[i].set(clock);
		
		// fire off the trigger on a 0 to 1 rising edge
		if (!prevPhase &&  dividers[i].phase) {
			pgTriggers[i].trigger(1e-3f);
		}
		
		// determine the output value
		bool trigOut = false;
		if (!muteAll && params[MUTE_PARAM + i].value < 0.5f ) {
			switch ((int)(params[OUTPUTMODE_PARAM].value)) {
				case 0: // trigger
					trigOut = pgTriggers[i].process(engineGetSampleTime());
					break;
				case 1: // gate
					trigOut = dividers[i].phase;
					break;
				case 2: // gated clock
					trigOut = gpClocks[i].high() &&  dividers[i].phase;
					break;
				case 3:	// clock
					trigOut = pgTriggers[i].process(engineGetSampleTime());
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
		
		// output the trigger value and set the light
		outputs[TRIG_OUTPUT + i].value = boolToGate(trigOut);
		lights[TRIG_LIGHT + i].setBrightnessSmooth(boolToLight(trigOut));
	
		// save these values for normalling in the next iteration 
		prevClock = clock;
		prevReset = res;
		prevCV = cv;
	}
}

struct PolyrhythmicGeneratorWidget : ModuleWidget {
	PolyrhythmicGeneratorWidget(PolyrhythmicGenerator *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/PolyrhythmicGenerator.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// division channels
		for (int i = 0; i < 8; i++) {
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CLOCK_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::RESET_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CV_INPUT + i));

			// knobs
			addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::CV_PARAM + i, -1.0f, 1.0f, 0.0f));
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL11], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::DIV_PARAM + i, 0.0f, 10.0f, 0.0f));
			
			// buttons
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL13]-10, STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::MUTE_PARAM + i, 0.0f, 1.0f, 0.0f));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL14], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::TRIG_LIGHT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL15], STD_ROWS8[STD_ROW1 + i]), module, PolyrhythmicGenerator::TRIG_OUTPUT + i));
		}
		
		// global stuff
		addParam(createParamCentered<CountModulaRotarySwitch5PosRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, PolyrhythmicGenerator::OUTPUTMODE_PARAM, 0.0f, 3.0f, 0.0f));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, PolyrhythmicGenerator::BEATMODE_INPUT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, PolyrhythmicGenerator::BEATMODE_PARAM, 0.0f, 1.0f, 1.0f));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, PolyrhythmicGenerator::MUTEALL_INPUT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, PolyrhythmicGenerator::MUTEALL_PARAM, 0.0f, 1.0f, 0.0f));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelPolyrhythmicGenerator = Model::create<PolyrhythmicGenerator, PolyrhythmicGeneratorWidget>("Count Modula", "PolyrhythmicGenerator", "Polyrhythmic Generator", CLOCK_MODULATOR_TAG);
