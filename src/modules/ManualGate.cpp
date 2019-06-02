//----------------------------------------------------------------------------
//	Count Modula - Manual Gate Module
//	A simple manual gate generator with a nice big button offering gate, latch
//	extended gate and trigger outputs 
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

struct ManualGate : Module {
	enum ParamIds {
		GATE_PARAM,
		LENGTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		IGATE_OUTPUT,
		EXTENDED_OUTPUT,
		IEXT_OUTPUT,
		TRIG_OUTPUT,
		LATCH_OUTPUT,
		ILATCH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LATCH_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gate;
	PulseGenerator  pgTrig;
	PulseGenerator  pgGate;
	bool latch = false;
	
	ManualGate() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	void onReset() override {
		latch = false;
		gate.reset();
		pgTrig.reset();
		pgGate.reset();
	}	
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void ManualGate::step() {
	
		gate.set(params[GATE_PARAM].value * 10.0f);
		
		if (gate.leadingEdge()) {
			latch = !latch;
		
			// fire off a trigger pulse
			pgTrig.trigger(1e-3f);
		}
		
		if (gate.high()) {
			// keep resetting the pulse extender until such time as the gate button goes low
			pgGate.trigger(params[LENGTH_PARAM].value);
		}
		
		float st = engineGetSampleTime();
		outputs[TRIG_OUTPUT].value = boolToGate(pgTrig.process(st));
		
		
		outputs[GATE_OUTPUT].value = gate.value();
		outputs[IGATE_OUTPUT].value = gate.notValue();
		
		outputs[LATCH_OUTPUT].value = boolToGate(latch);
		outputs[ILATCH_OUTPUT].value = boolToGate(!latch);
		
		bool ext = (gate.high() || pgGate.process(st));
		outputs[EXTENDED_OUTPUT].value = boolToGate(ext);
		outputs[IEXT_OUTPUT].value = boolToGate(!ext);
		
		lights[LATCH_LIGHT].setBrightnessSmooth(boolToLight(latch));
		
}

struct ManualGateWidget : ModuleWidget {
	ManualGateWidget(ManualGate *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/ManualGate.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, ManualGate::LENGTH_PARAM, 0.0f, 10.0f, 0.0f));

	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualGate::GATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, ManualGate::IGATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, ManualGate::LATCH_OUTPUT));		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, ManualGate::ILATCH_OUTPUT));		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, ManualGate::EXTENDED_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, ManualGate::IEXT_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, ManualGate::TRIG_OUTPUT));	
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW2]), module, ManualGate::LATCH_LIGHT));

		// Mega manual button - non-standard position
		addParam(createParamCentered<CountModulaPBSwitchMegaMomentary>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW7]), module, ManualGate::GATE_PARAM, 0.0f, 1.0f, 0.0f));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelManualGate = Model::create<ManualGate, ManualGateWidget>("Count Modula", "ManualGate", "Manual Gate", CONTROLLER_TAG);
