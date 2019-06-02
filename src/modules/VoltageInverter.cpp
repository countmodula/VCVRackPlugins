//----------------------------------------------------------------------------
//	Count Modula - Voltage Inverter Module
//	A basic quad voltage inverter
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct VoltageInverter : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	float out = 0.0f;
	VoltageInverter() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void VoltageInverter::step() {
	out = 0.0f;
	for (int i = 0; i < 4; i++) {
		// grab output value normalised to previous input value
		out = inputs[A_INPUT + i].normalize(out);
		outputs[A_OUTPUT + i].value = -out;
	}
}

struct VoltageInverterWidget : ModuleWidget {
VoltageInverterWidget(VoltageInverter *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/VoltageInverter.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 4; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (i * 2)]), module, VoltageInverter::A_INPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (i * 2)]), module, VoltageInverter::A_OUTPUT + (i)));
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelVoltageInverter = Model::create<VoltageInverter, VoltageInverterWidget>("Count Modula", "VoltageInverter", "Voltage Inverter", SAMPLE_AND_HOLD_TAG, SEQUENCER_TAG);
