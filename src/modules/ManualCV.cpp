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
	
	ManualCV() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void ManualCV::step() {
	outputs[CV1_OUTPUT].value = clamp(params[CV1COARSE_PARAM].value + params[CV1FINE_PARAM].value, -10.0f, 10.0f);
	outputs[CV2_OUTPUT].value = clamp(params[CV2COARSE_PARAM].value + params[CV2FINE_PARAM].value, -10.0f, 10.0f);
}

struct ManualCVWidget : ModuleWidget {
	ManualCVWidget(ManualCV *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/ManualCV.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, ManualCV::CV1COARSE_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, ManualCV::CV1FINE_PARAM, -0.5f, 0.5f, 0.0f));

		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, ManualCV::CV2COARSE_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, ManualCV::CV2FINE_PARAM, -0.5f, 0.5f, 0.0f));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualCV::CV1_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, ManualCV::CV2_OUTPUT));	
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelManualCV = Model::create<ManualCV, ManualCVWidget>("Count Modula", "ManualCV", "Manual CV", UTILITY_TAG);
