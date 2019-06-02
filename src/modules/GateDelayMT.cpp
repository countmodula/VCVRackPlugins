//----------------------------------------------------------------------------
//	/^M^\	Count Modula - Multi-tapped Gate Delay Module
//	A shift register style gate delay offering 8 tapped gate outputs 
//----------------------------------------------------------------------------
#include <queue>
#include <deque>

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/GateDelayLine.hpp"

struct GateDelayMT : Module {
	enum ParamIds {
		TIME_PARAM,
		CVLEVEL_PARAM,
		RANGE_PARAM,
		MIXDIR_PARAM,
		ENUMS(MIXDEL_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		TIME_INPUT,
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DIRECT_OUTPUT,
		ENUMS(DELAYED_OUTPUTS, 8),
		MIX_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		DIRECT_LIGHT,
		ENUMS(DELAYED_LIGHTS, 8),
		MIX_LIGHT,
		NUM_LIGHTS
	}; 

	GateDelayLine delayLine;
	GateDelayMT() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

	int range = 0;
	
	int taps[3][8] = {
		{ 4,  8, 12, 16, 20, 24, 28, 32}, 
		{ 2,  4,  6,  8, 10, 12, 14, 16},
		{ 1,  2,  3,  4,  5,  6,  7,  8}
	};
	
	void step() override;
	
	void onReset() override {
		delayLine.reset();
	}
};

void GateDelayMT::step() {

	// determine the range option 
	int t = (int)(params[RANGE_PARAM].value);
	if (range != t) {
#ifdef NO_TIME_TRAVEL		
		// range has changed - clear out the delay line otherwise we can get some undesirable time travel artefacts
		delayLine.reset();
#endif
		range = t;
	}

	// compute delay time in seconds
	float delay = params[TIME_PARAM].value;
	if (inputs[TIME_INPUT].active)
		 delay += (inputs[TIME_INPUT].value * params[CVLEVEL_PARAM].value);

	 // process the delay and grab the input gate level
	delayLine.process(inputs[GATE_INPUT].value, delay);

	float mix = 0.0f;
	
	// direct output
	outputs[DIRECT_OUTPUT].value = boolToGate(delayLine.gateValue());
	lights[DIRECT_LIGHT].setBrightnessSmooth(outputs[DIRECT_OUTPUT].value / 10.0f);
	
	// mix direct in if we want it
	mix += (params[MIXDIR_PARAM].value * outputs[DIRECT_OUTPUT].value);
	
	for (int i = 0; i < 8; i++) {
		outputs[DELAYED_OUTPUTS + i].value = boolToGate(delayLine.tapValue(taps[range][i]));
		lights[DELAYED_LIGHTS + i].setBrightnessSmooth(outputs[DELAYED_OUTPUTS + i].value / 10.0f);
		
		// add this tap to the mix
		mix += (params[MIXDEL_PARAMS + i].value * outputs[DELAYED_OUTPUTS + i].value);
	}
	
	// finally output the mix 
	outputs[MIX_OUTPUT].value = boolToGate(mix > 0.1f);
	lights[MIX_LIGHT].setBrightnessSmooth(boolToLight(mix > 0.1f));
}

struct GateDelayMTWidget : ModuleWidget { 
	GateDelayMTWidget(GateDelayMT *module) : ModuleWidget(module) {

		setPanel(SVG::load(assetPlugin(plugin, "res/GateDelayMT.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// parameters 
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, GateDelayMT::CVLEVEL_PARAM, -5.0f, 5.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]), module, GateDelayMT::TIME_PARAM, 0.0f, 10.0f, 5.0f));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW2]), module, GateDelayMT::RANGE_PARAM, 0.0f, 2.0f, 2.0f));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, GateDelayMT::TIME_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, GateDelayMT::GATE_INPUT));

		// direct output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, GateDelayMT::DIRECT_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW2]), module, GateDelayMT::DIRECT_LIGHT));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIXDIR_PARAM, 0.0f, 1.0f, 1.0f));
			
		// mix output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIX_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW2]), module, GateDelayMT::MIX_LIGHT));
		
		// tapped outputs
		int k = 0;
		for (int i = 0; i < 5; i += 4) {
			for (int j = 0; j < 4; j++) {
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::DELAYED_OUTPUTS + k));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::DELAYED_LIGHTS + k));
				addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + i], STD_ROWS6[STD_ROW3 + j]), module, GateDelayMT::MIXDEL_PARAMS + k, 0.0f, 1.0f, 1.0f));
			
				k++;
			}
		}
	}
};

Model *modelGateDelayMT = Model::create<GateDelayMT, GateDelayMTWidget>("Count Modula", "GateDelayMT", "Multi-tapped Gate Delay", DELAY_TAG);