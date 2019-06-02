//----------------------------------------------------------------------------
//	Count Modula - Voltage Controlled Switch Module
//	A 2 in/1 out 1 in/2 out voltage controlled switch
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "dsp/digital.hpp"

struct VoltageControlledSwitch : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		A_INPUT,
		B1_INPUT,
		B2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A1_OUTPUT,
		A2_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A1_LIGHT,
		A2_LIGHT,
		B1_LIGHT,
		B2_LIGHT,
		NUM_LIGHTS
	};

	SchmittTrigger stSwitch;
	
	VoltageControlledSwitch() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	void onReset() override {
		stSwitch.reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void VoltageControlledSwitch::step() {
	
	float cv = inputs[CV_INPUT].normalize(0.0f);
	
	// not using standard gate processor here due to different trigger levels
	stSwitch.process(rescale(cv, 3.2f, 3.6f, 0.0f, 1.0f));

	if (stSwitch.isHigh()) {
		// select I/O 2
		outputs[A1_OUTPUT].value = 0.0f;
		outputs[A2_OUTPUT].value = inputs[A_INPUT].value;
		outputs[B_OUTPUT].value = inputs[B2_INPUT].value;
		
		lights[A1_LIGHT].setBrightnessSmooth(0.0f);
		lights[A2_LIGHT].setBrightnessSmooth(1.0f);
		lights[B1_LIGHT].setBrightnessSmooth(0.0f);
		lights[B2_LIGHT].setBrightnessSmooth(1.0f);
	}
	else {
		// select I/O 1
		outputs[A1_OUTPUT].value = inputs[A_INPUT].value;
		outputs[A2_OUTPUT].value = 0.0f;
		outputs[B_OUTPUT].value = inputs[B1_INPUT].value;
		
		lights[A1_LIGHT].setBrightnessSmooth(1.0f);
		lights[A2_LIGHT].setBrightnessSmooth(0.0f);
		lights[B1_LIGHT].setBrightnessSmooth(1.0f);
		lights[B2_LIGHT].setBrightnessSmooth(0.0f);
	}
}

struct VoltageControlledSwitchWidget : ModuleWidget {
VoltageControlledSwitchWidget(VoltageControlledSwitch *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/VoltageControlledSwitch.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, VoltageControlledSwitch::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, VoltageControlledSwitch::A_INPUT));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, VoltageControlledSwitch::B1_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, VoltageControlledSwitch::B2_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, VoltageControlledSwitch::A1_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, VoltageControlledSwitch::A2_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, VoltageControlledSwitch::B_OUTPUT));

		// lights
		int ledColumn = 2 + ((STD_COLUMN_POSITIONS[STD_COL1] + STD_COLUMN_POSITIONS[STD_COL2])/ 2);
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW2) + 4), module, VoltageControlledSwitch::A1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW3) + 4), module, VoltageControlledSwitch::A2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW4) + 5), module, VoltageControlledSwitch::B1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW5) + 5), module, VoltageControlledSwitch::B2_LIGHT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelVoltageControlledSwitch = Model::create<VoltageControlledSwitch, VoltageControlledSwitchWidget>("Count Modula", "VoltageControlledSwitch", "Voltage Controlled Switch", SWITCH_TAG);
