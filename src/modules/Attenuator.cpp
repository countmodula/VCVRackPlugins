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

	Attenuator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CH1_ATTENUATION_PARAM, 0.0f, 1.0f, 0.5f, "Attenuation/Attenuversion");
		configParam(CH2_ATTENUATION_PARAM, 0.0f, 1.0f, 0.0f, "Attenuation");
		configParam(CH1_MODE_PARAM, 0.0f, 1.0f, 0.0f, "Attenuvert");
	}

	void onReset() override {
		polarizer.reset();
	}
	
	void process(const ProcessArgs &args) override {
		float out1 = inputs[CH1_SIGNAL_INPUT].getNormalVoltage(10.0f);
		float out2 = inputs[CH2_SIGNAL_INPUT].getNormalVoltage(10.0f);
		
		if (params[CH1_MODE_PARAM].getValue() > 0.5f)
			outputs[CH1_SIGNAL_OUTPUT].setVoltage(polarizer.process(out1, -1.0f + (params[CH1_ATTENUATION_PARAM].getValue() * 2), 0.0f, 0.0f));
		else
			outputs[CH1_SIGNAL_OUTPUT].setVoltage(out1 * params[CH1_ATTENUATION_PARAM].getValue());

		outputs[CH2_SIGNAL_OUTPUT].setVoltage(out2 * params[CH2_ATTENUATION_PARAM].getValue());
	}
};

struct AttenuatorWidget : ModuleWidget {
	AttenuatorWidget(Attenuator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Attenuator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, Attenuator::CH1_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, Attenuator::CH2_SIGNAL_INPUT));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, Attenuator::CH1_ATTENUATION_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, Attenuator::CH2_ATTENUATION_PARAM));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, Attenuator::CH1_SIGNAL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Attenuator::CH2_SIGNAL_OUTPUT));
	
		// switches
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, Attenuator::CH1_MODE_PARAM));
	}
};


Model *modelAttenuator = createModel<Attenuator, AttenuatorWidget>("Attenuator");
