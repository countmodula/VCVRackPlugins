//----------------------------------------------------------------------------
//	Count Modula - Comparator Module
//	An analogue comparator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct Comparator : Module {
	enum ParamIds {
		THRESHOLD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		THRESHOLD_INPUT,
		COMPARE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		COMPARE_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OVER_LIGHT,
		UNDER_LIGHT,
		NUM_LIGHTS
	};

	bool state = 0;

	Comparator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void Comparator::step() {

	// Compute the threshold from the pitch parameter and input
	float threshold = params[THRESHOLD_PARAM].value;
	threshold += inputs[THRESHOLD_INPUT].value;

	float compare = inputs[COMPARE_INPUT].value;
	
	// compare
	state = (compare > threshold);
	
	if (state) {
		// set the output high
		outputs[COMPARE_OUTPUT].value = 10.0f;
		outputs[INV_OUTPUT].value = 0.0f;

		// Set light off
		lights[OVER_LIGHT].value = 1.0f;
		lights[UNDER_LIGHT].value = 0.0f;
		}
	else {
		// set the output low
		outputs[COMPARE_OUTPUT].value = 0.0f;
		outputs[INV_OUTPUT].value = 10.0f;

		// Set light off
		lights[UNDER_LIGHT].value = 1.0f;
		lights[OVER_LIGHT].value = 0.0f;
	}
}

struct ComparatorWidget : ModuleWidget {
ComparatorWidget(Comparator *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Comparator.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CountModulaKnobGreen>(Vec(45, 75), module, Comparator::THRESHOLD_PARAM, -10.0, 10.0, 0.0));

		addInput(createInputCentered<CountModulaJack>(Vec(45, 130), module, Comparator::THRESHOLD_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(45, 180), module, Comparator::COMPARE_INPUT));

		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 260), module, Comparator::COMPARE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 307), module, Comparator::INV_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 260), module, Comparator::OVER_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 307), module, Comparator::UNDER_LIGHT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelComparator = Model::create<Comparator, ComparatorWidget>("Count Modula", "Comparator", "Comparator", UTILITY_TAG);
