//----------------------------------------------------------------------------
//	/^M^\	Count Modula - Gate Delay Module
//	A shift register style gate delay offering up to 20 seconds of
//  delay
//----------------------------------------------------------------------------
#include <queue>
#include <deque>

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/GateDelayLine.hpp"

struct GateDelay : Module {
	enum ParamIds {
		ENUMS(TIME_PARAM, 2),
		ENUMS(CVLEVEL_PARAM, 2),
		ENUMS(RANGE_PARAM, 2),
		DUMMY_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(TIME_INPUT, 2),
		ENUMS(GATE_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(GATE_OUTPUT, 2),
		ENUMS(DELAYED_OUTPUT, 2),
		ENUMS(MIX_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATE_OUT_LIGHT, 2),
		ENUMS(DELAYED_OUT_LIGHT, 2),
		ENUMS(MIX_OUT_LIGHT, 2),
		NUM_LIGHTS
	}; 

	float delayedGate[2] = {0.0f, 0.0f};
	float gateIn[2] = {0.0f, 0.0f};

	int taps [5] = {2, 4, 8, 16, 32};

	int range[2] = {0, 0};
	
	GateDelayLine delayLine[2];
	
	GateDelay() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	void step() override;
	
	void onReset() override {
		for (int i = 0; i < 2; i++) {
			gateIn[i] = delayedGate[i] = 0.0f;
			delayLine[i].reset();
		}
	}
};

void GateDelay::step() {

	for (int i = 0; i < 2; i++) {
		
		// determine the range option 
		int t = taps[(int)(params[RANGE_PARAM + i].value)];
		if (range[i] != t) {
#ifdef NO_TIME_TRAVEL		
			// range has changed - clear out the delay line otherwise we can get some undesirable time travel artefacts
			delayLine[i].reset();
#endif
			range[i] = t;
		}	
		
		// compute delay time in seconds
		float delay = params[TIME_PARAM + i].value;
		if (inputs[TIME_INPUT + i].active)
			 delay += (inputs[TIME_INPUT + i].value * params[CVLEVEL_PARAM + i].value);

		 // process the delay and grab the input gate level
		delayLine[i].process(inputs[GATE_INPUT + i].value, delay);
		gateIn[i] = boolToGate(delayLine[i].gateValue());
		
		// handle the selected range
		delayedGate[i] = boolToGate(delayLine[i].tapValue(range[i]));
		
		// set the outputs
		outputs[GATE_OUTPUT + i].value = gateIn[i];
		outputs[DELAYED_OUTPUT + i].value = delayedGate[i];
		outputs[MIX_OUTPUT + i].value = clamp(delayedGate[i] + gateIn[i], 0.0f, 10.0f);
		
		// set the lights
		lights[GATE_OUT_LIGHT + i].setBrightnessSmooth(outputs[GATE_OUTPUT + i].value / 10.0f);
		lights[DELAYED_OUT_LIGHT + i].setBrightnessSmooth(outputs[DELAYED_OUTPUT + i].value / 10.0f);
		lights[MIX_OUT_LIGHT + i].setBrightnessSmooth(outputs[MIX_OUTPUT + i].value / 10.0f);
	}
}

struct GateDelayWidget : ModuleWidget { 
	GateDelayWidget(GateDelay *module) : ModuleWidget(module) {

		setPanel(SVG::load(assetPlugin(plugin, "res/GateDelay.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int i = 0; i < 2; i++) {
			int j = i * 3;
			
			// parameters 
			if (i == 0) {
				addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, GateDelay::CVLEVEL_PARAM + i, -5.0f, 5.0f, 0.0f));
				addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1 + j]), module, GateDelay::TIME_PARAM + i, 0.0f, 10.0f, 5.0f));
				addParam(createParamCentered<CountModulaRotarySwitch5PosYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2 + j]), module, GateDelay::RANGE_PARAM + i, 0.0f, 4.0f, 0.0f));
			}
			else {
				addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, GateDelay::CVLEVEL_PARAM + i, -5.0f, 5.0f, 0.0f));
				addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1 + j]), module, GateDelay::TIME_PARAM + i, 0.0f, 10.0f, 5.0f));
				addParam(createParamCentered<CountModulaRotarySwitch5PosWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2 + j]), module, GateDelay::RANGE_PARAM + i, 0.0f, 4.0f, 0.0f));
			}
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, GateDelay::TIME_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + j]), module, GateDelay::GATE_INPUT + i));
			
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + j]), module, GateDelay::GATE_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + j]), module, GateDelay::MIX_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3 + j]), module, GateDelay::DELAYED_OUTPUT + i));

			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 18, STD_ROWS6[STD_ROW3 + j] - 21), module, GateDelay::GATE_OUT_LIGHT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + j] - 21), module, GateDelay::MIX_OUT_LIGHT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 18, STD_ROWS6[STD_ROW3 + j] - 21), module, GateDelay::DELAYED_OUT_LIGHT + i));
		}
	}
};

Model *modelGateDelay = Model::create<GateDelay, GateDelayWidget>("Count Modula", "GateDelay", "Gate Delay", DELAY_TAG);