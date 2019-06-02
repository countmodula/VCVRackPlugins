//----------------------------------------------------------------------------
//	Count Modula - CV Spreader Module
//	Based on the CGS37 CV Cluster by Ken Stone.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct CVSpreader : Module {
	enum ParamIds {
		BASE_PARAM,
		SPREAD_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		BASE_INPUT,
		SPREAD_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		E_OUTPUT,
		F_OUTPUT,
		G_OUTPUT,
		H_OUTPUT,
		I_OUTPUT,
		J_OUTPUT,
		K_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	CVSpreader() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void CVSpreader::step() {

	float base = inputs[BASE_INPUT].normalize(10.0f) * params[BASE_PARAM].value;
	float spread = inputs[SPREAD_INPUT].normalize(5.0f) * params[SPREAD_PARAM].value;
	float even = (params[MODE_PARAM].value < 0.5f ? 0.0f : 1.0f);
	
	float pos = base + spread;
	float neg = base - spread;
	float diff = 2.0f * spread;
	float div = diff / (9.0f + (even ? 1.0f : 0.0f));

	// output F is always the base value
	outputs[F_OUTPUT].value = base;
	
	for (int i = 0; i < 5; i++) {
		// pos outputs
		outputs[E_OUTPUT - i].value = clamp(pos - ((float)i * div), -10.0f, 10.0f);
		
		// neg outputs
		outputs[K_OUTPUT - i].value = clamp(neg + ((float)i * div), -10.0f, 10.0f);
 	}
}

struct CVSpreaderWidget : ModuleWidget {
	CVSpreaderWidget(CVSpreader *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/CVSpreader.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// CV knobs
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, CVSpreader::BASE_PARAM, -1.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, CVSpreader::SPREAD_PARAM, -1.0f, 1.0f, 0.0f));
	
		// odd/even switch
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS6(STD_ROW5)), module, CVSpreader::MODE_PARAM, 0.0f, 1.0f, 1.0f));
		
		// base and spread input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, CVSpreader::BASE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, CVSpreader::SPREAD_INPUT));
		
		// base output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW1]), module, CVSpreader::F_OUTPUT)); // Base
		
		// Sum outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, CVSpreader::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, CVSpreader::B_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, CVSpreader::C_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, CVSpreader::D_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, CVSpreader::E_OUTPUT));
		
		// Diff outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, CVSpreader::G_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, CVSpreader::H_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, CVSpreader::I_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, CVSpreader::J_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, CVSpreader::K_OUTPUT));	
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelCVSpreader = Model::create<CVSpreader, CVSpreaderWidget>("Count Modula", "CVSpreader", "CV Spreader", UTILITY_TAG);
