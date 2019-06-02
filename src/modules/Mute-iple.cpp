//----------------------------------------------------------------------------
//	Count Modula - Muteble Module
//	A mutable multiple.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "dsp/digital.hpp"

struct MuteIple : Module {
	enum ParamIds {
		ENUMS(MUTE_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(SIGNAL_INPUTS, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SIGNAL_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	MuteIple() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void MuteIple::step() {
	// grab the inputs`
	float in1 = inputs[SIGNAL_INPUTS].value;
	float in2 = inputs[SIGNAL_INPUTS + 1].normalize(in1);
	
	// send 'em to the outputs
	for (int i = 0; i < 8; i++) {
		outputs[SIGNAL_OUTPUTS + i].value = (1.0f - params[MUTE_PARAMS + i].value) * (i < 4 ? in1 : in2);
	}
}

struct MuteIpleWidget : ModuleWidget {
MuteIpleWidget(MuteIple *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/Mute-iple.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, MuteIple::SIGNAL_INPUTS));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, MuteIple::SIGNAL_INPUTS + 1));
		
		// outputs/lights
		for (int i = 0; i < 8 ; i++) {
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::MUTE_PARAMS + i, 0.0f, 1.0f, 0.0f));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::SIGNAL_OUTPUTS + i));	
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMuteIple = Model::create<MuteIple, MuteIpleWidget>("Count Modula", "Mute-iple", "Mute-iple", MULTIPLE_TAG, CONTROLLER_TAG);
