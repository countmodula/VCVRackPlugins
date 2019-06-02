//----------------------------------------------------------------------------
//	Count Modula - Voltage Controlled Polarizer Module
//	AA 2 channel voltage controlled signal polarizer
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

struct VCPolarizer : Module {
	enum ParamIds {
		CH1_CVAMOUNT_PARAM,
		CH1_MANUAL_PARAM,
		CH2_CVAMOUNT_PARAM,
		CH2_MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_CV_INPUT,
		CH1_SIGNAL_INPUT,
		CH2_CV_INPUT,
		CH2_SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH1_SIGNAL_OUTPUT,
		CH2_SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CH1_POS_LIGHT,
		CH1_NEG_LIGHT,
		CH2_POS_LIGHT,
		CH2_NEG_LIGHT,
		NUM_LIGHTS
	};

	Polarizer polarizer1;
	Polarizer polarizer2;

	VCPolarizer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;

	void onReset() override {
		polarizer1.reset();
		polarizer2.reset();
	}	
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void VCPolarizer::step() {
	// channel 1
	outputs[CH1_SIGNAL_OUTPUT].value = polarizer1.process(inputs[CH1_SIGNAL_INPUT].value, params[CH1_MANUAL_PARAM].value, inputs[CH1_CV_INPUT].value, params[CH1_CVAMOUNT_PARAM].value);
	lights[CH1_POS_LIGHT].setBrightnessSmooth(polarizer1.positiveLevel);
	lights[CH1_NEG_LIGHT].setBrightnessSmooth(polarizer1.negativeLevel);
	
	// channel 2
	outputs[CH2_SIGNAL_OUTPUT].value = polarizer2.process(inputs[CH2_SIGNAL_INPUT].value, params[CH2_MANUAL_PARAM].value, inputs[CH2_CV_INPUT].value, params[CH2_CVAMOUNT_PARAM].value);
	lights[CH2_POS_LIGHT].setBrightnessSmooth(polarizer2.positiveLevel);
	lights[CH2_NEG_LIGHT].setBrightnessSmooth(polarizer2.negativeLevel);
}

struct VCPolarizerWidget : ModuleWidget {
VCPolarizerWidget(VCPolarizer *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/VCPolarizer.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, VCPolarizer::CH1_CVAMOUNT_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, VCPolarizer::CH1_MANUAL_PARAM, -2.0f, 2.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, VCPolarizer::CH2_CVAMOUNT_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, VCPolarizer::CH2_MANUAL_PARAM, -2.0f, 2.0f, 0.0f));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCPolarizer::CH1_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCPolarizer::CH2_CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCPolarizer::CH1_SIGNAL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCPolarizer::CH2_SIGNAL_OUTPUT));

		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_NEG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 10, STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_POS_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_NEG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 10, STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_POS_LIGHT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelVCPolarizer = Model::create<VCPolarizer, VCPolarizerWidget>("Count Modula", "VCPolarizer", "VC Polarizer", UTILITY_TAG);
