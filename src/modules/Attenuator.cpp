//----------------------------------------------------------------------------
//	/^M^\	Count Modula - Attenuator Module
//	A basic dual attenuator module with one switchable attenuverter and one 
//  simple attenuator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

struct Attenuator : Module {
	enum ParamIds {
		CH1_ATTENUATION_PARAM,
		CH1_MODE_PARAM,
		CH2_ATTENUATION_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_SIGNAL_INPUT,
		CH2_SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH1_SIGNAL_OUTPUT,
		CH2_SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Polarizer polarizer;

	Attenuator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	void onReset() override {
		polarizer.reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void Attenuator::step() {
	float out1 = inputs[CH1_SIGNAL_INPUT].normalize(10.0f);
	float out2 = inputs[CH2_SIGNAL_INPUT].normalize(10.0f);
	
	if (params[CH1_MODE_PARAM].value > 0.5f)
		outputs[CH1_SIGNAL_OUTPUT].value = polarizer.process(out1, -1.0f + (params[CH1_ATTENUATION_PARAM].value * 2), 0.0f, 0.0f);
	else
		outputs[CH1_SIGNAL_OUTPUT].value = out1 * params[CH1_ATTENUATION_PARAM].value;

	outputs[CH2_SIGNAL_OUTPUT].value = out2 * params[CH2_ATTENUATION_PARAM].value;
}

struct AttenuatorWidget : ModuleWidget {
	AttenuatorWidget(Attenuator *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Attenuator.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, Attenuator::CH1_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, Attenuator::CH2_SIGNAL_INPUT));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, Attenuator::CH1_ATTENUATION_PARAM, 0.0f, 1.0f, 0.5f));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, Attenuator::CH2_ATTENUATION_PARAM, 0.0f, 1.0f, 0.0f));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, Attenuator::CH1_SIGNAL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Attenuator::CH2_SIGNAL_OUTPUT));
	
		// switches
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, Attenuator::CH1_MODE_PARAM, 0.0f, 1.0f, 0.0f));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelAttenuator = Model::create<Attenuator, AttenuatorWidget>("Count Modula", "Attenuator", "Attenuator", ATTENUATOR_TAG);
