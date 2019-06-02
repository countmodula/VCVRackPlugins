//----------------------------------------------------------------------------
//	Count Modula - G2T Module
//	Gate To Trigger Module
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Inverter.hpp"

struct G2T : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		INV_OUTPUT,
		START_OUTPUT,
		END_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		GATE_LIGHT,
		START_LIGHT,
		END_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gate;
	PulseGenerator pgStart;
	PulseGenerator pgEnd;

	G2T() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;

	void onReset() override {
		gate.reset();
		pgStart.reset();
		pgEnd.reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void G2T::step() {
	
	float sampleTime = engineGetSampleTime();
	
	// process the input
	gate.set(inputs[GATE_INPUT].value);

		// leading edge - fire the start trigger
	if (gate.leadingEdge())
		pgStart.trigger(1e-3f);

	// find leading/trailing edge
	if (gate.trailingEdge())
		pgEnd.trigger(1e-3f);
	
	// process the gate outputs
	if (gate.high()) {
		outputs[GATE_OUTPUT].value = 10.0f; 
		outputs[INV_OUTPUT].value = 0.0;
		lights[GATE_INPUT].setBrightnessSmooth(10.0f);
	}
	else {
		outputs[GATE_OUTPUT].value = 0.0f; 
		outputs[INV_OUTPUT].value = 10.0; 
		lights[GATE_INPUT].setBrightnessSmooth(0.0f);
	}
	
	// process the start trigger
	if (pgStart.process(sampleTime)) {
		outputs[START_OUTPUT].value = 10.0f;
		lights[START_LIGHT].setBrightnessSmooth(10.0f);
	}
	else {
		outputs[START_OUTPUT].value = 0.0f;
		lights[START_LIGHT].setBrightnessSmooth(0.0f);
	}

	// process the end trigger
	if (pgEnd.process(sampleTime)) {
		outputs[END_OUTPUT].value = 10.0f;
		lights[END_LIGHT].setBrightnessSmooth(10.0f);
	}
	else {
		outputs[END_OUTPUT].value = 0.0f;
		lights[END_LIGHT].setBrightnessSmooth(0.0f);
	}
}

struct G2TWidget : ModuleWidget {
G2TWidget(G2T *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/G2T.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1] + 12), module, G2T::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, G2T::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, G2T::INV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 6), module, G2T::START_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 12), module, G2T::END_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2] + 15), module, G2T::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5] + 6), module, G2T::START_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, G2T::END_LIGHT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelG2T = Model::create<G2T, G2TWidget>("Count Modula", "G2T", "Gate To Trigger", LOGIC_TAG);
