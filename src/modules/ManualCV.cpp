//----------------------------------------------------------------------------
//	Count Modula - Manual CV Module
//	A dual manual CV generator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct ManualCV : Module {
	enum ParamIds {
		CV1COARSE_PARAM,
		CV1FINE_PARAM,
		CV2COARSE_PARAM,
		CV2FINE_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		CV1_OUTPUT,
		CV2_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	float outVal;
	
	ManualCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV1COARSE_PARAM, -10.0f, 10.0f, 0.0f, "Coarse value", " V");
		configParam(CV1FINE_PARAM, -0.5f, 0.5f, 0.0f, "Fine value", " V");
		configParam(CV2COARSE_PARAM, -10.0f, 10.0f, 0.0f, "Coarse value", " V");
		configParam(CV2FINE_PARAM, -0.5f, 0.5f, 0.0f, "Fine value", " V");
	}
	
	void process(const ProcessArgs &args) override {
		outputs[CV1_OUTPUT].setVoltage(clamp(params[CV1COARSE_PARAM].getValue() + params[CV1FINE_PARAM].getValue(), -10.0f, 10.0f));
		outputs[CV2_OUTPUT].setVoltage(clamp(params[CV2COARSE_PARAM].getValue() + params[CV2FINE_PARAM].getValue(), -10.0f, 10.0f));
	}
};

struct ManualCVWidget : ModuleWidget {
	ManualCVWidget(ManualCV *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ManualCV.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, ManualCV::CV1COARSE_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, ManualCV::CV1FINE_PARAM));

		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, ManualCV::CV2COARSE_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, ManualCV::CV2FINE_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualCV::CV1_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, ManualCV::CV2_OUTPUT));	
	}
};

Model *modelManualCV = createModel<ManualCV, ManualCVWidget>("ManualCV");
