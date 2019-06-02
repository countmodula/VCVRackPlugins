//----------------------------------------------------------------------------
//	Count Modula - Voltage Controlled Divider Module
//	A voltage controlled frequency divider (divide by 1 - approx 20)
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/Utility.hpp"

struct VCFrequencyDivider : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		DIV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DIVB_OUTPUT,
		DIVU_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	FrequencyDivider divider;
	
	VCFrequencyDivider() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	void onReset() override {
		divider.reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void VCFrequencyDivider::step() {

	divider.setMaxN(20);
	divider.setCountMode(COUNT_DN);
	
	float div = params[MANUAL_PARAM].value + (params[CV_PARAM].value * inputs[CV_INPUT].normalize(0.0f));
	divider.setN(div);
	divider.process(inputs[DIV_INPUT].normalize(0.0f));
	
	outputs[DIVB_OUTPUT].value = boolToAudio(divider.phase);
	outputs[DIVU_OUTPUT].value = boolToGate(divider.phase);
}

struct VCFrequencyDividerWidget : ModuleWidget {
VCFrequencyDividerWidget(VCFrequencyDivider *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/VCFrequencyDivider.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCFrequencyDivider::CV_PARAM, -1.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCFrequencyDivider::MANUAL_PARAM, 0.0f, 10.0f, 0.0f));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, VCFrequencyDivider::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCFrequencyDivider::DIV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCFrequencyDivider::DIVB_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCFrequencyDivider::DIVU_OUTPUT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelVCFrequencyDivider = Model::create<VCFrequencyDivider, VCFrequencyDividerWidget>("Count Modula", "VCFrequencyDivider", "VC Frequency Divider", UTILITY_TAG, CLOCK_MODULATOR_TAG);
