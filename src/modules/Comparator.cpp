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

	Comparator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(THRESHOLD_PARAM, -10.0, 10.0, 0.0, "Threshold", " V");
	}
	
	void process(const ProcessArgs &args) override {

		// Compute the threshold from the pitch parameter and input
		float threshold = params[THRESHOLD_PARAM].getValue();
		threshold += inputs[THRESHOLD_INPUT].getVoltage();

		float compare = inputs[COMPARE_INPUT].getVoltage();
		
		// compare
		state = (compare > threshold);
		
		if (state) {
			// set the output high
			outputs[COMPARE_OUTPUT].setVoltage(10.0f);
			outputs[INV_OUTPUT].setVoltage(0.0f);

			// Set light off
			lights[OVER_LIGHT].setBrightness(1.0f);
			lights[UNDER_LIGHT].setBrightness(0.0f);
		}
		else {
			// set the output low
			outputs[COMPARE_OUTPUT].setVoltage(0.0f);
			outputs[INV_OUTPUT].setVoltage(10.0f);

			// Set light off
			lights[UNDER_LIGHT].setBrightness(1.0f);
			lights[OVER_LIGHT].setBrightness(0.0f);
		}
	}
};



struct ComparatorWidget : ModuleWidget {
ComparatorWidget(Comparator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Comparator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CountModulaKnobGreen>(Vec(45, 75), module, Comparator::THRESHOLD_PARAM));

		addInput(createInputCentered<CountModulaJack>(Vec(45, 130), module, Comparator::THRESHOLD_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(45, 180), module, Comparator::COMPARE_INPUT));

		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 260), module, Comparator::COMPARE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(25, 307), module, Comparator::INV_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 260), module, Comparator::OVER_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(65, 307), module, Comparator::UNDER_LIGHT));
	}
};

Model *modelComparator = createModel<Comparator, ComparatorWidget>("Comparator");
