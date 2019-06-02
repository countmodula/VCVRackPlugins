//----------------------------------------------------------------------------
//	Count Modula - VC NOT Logic Gate Module
//	A Voltage controlled logical inverter
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"


struct BooleanVCNOT : Module {
	enum ParamIds {
		ENUMS(ENABLE_PARAM, 2),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(LOGIC_INPUT, 2),
		ENUMS(ENABLE_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(INV_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Inverter inverter[2];
	
	BooleanVCNOT() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;

	void onReset() override {
		for (int i = 0; i < 2; i++)
			inverter[i].reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void BooleanVCNOT::step() {
	//perform the logic
	for (int i = 0; i < 2; i++) {
		float out =  inverter[i].process(inputs[LOGIC_INPUT + i].value, inputs[ENABLE_INPUT + i].normalize(10.0f));
		outputs[INV_OUTPUT + i].value = out;
	}
}

struct BooleanVCNOTWidget : ModuleWidget {
	BooleanVCNOTWidget(BooleanVCNOT *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/BooleanVCNOT.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 2; i++) {
			// logic and enable inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + (i * 3)]),module, BooleanVCNOT::LOGIC_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + (i * 3)]),module, BooleanVCNOT::ENABLE_INPUT + i));
		
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + (i * 3)]), module, BooleanVCNOT::INV_OUTPUT + i));
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelBooleanVCNOT = Model::create<BooleanVCNOT, BooleanVCNOTWidget>("Count Modula", "BooleanVCNOT", "Logical VC Inverter", LOGIC_TAG);
