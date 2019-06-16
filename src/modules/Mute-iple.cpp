//----------------------------------------------------------------------------
//	Count Modula - Muteble Module
//	A mutable multiple.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

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

	MuteIple() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		for (int i = 0; i < 8 ; i++) {
			configParam(MUTE_PARAMS + i, 0.0f, 1.0f, 0.0f);
		}
	}
	
	void process(const ProcessArgs &args) override {

		// grab the inputs`
		float in1 = inputs[SIGNAL_INPUTS].getVoltage();
		float in2 = inputs[SIGNAL_INPUTS + 1].getNormalVoltage(in1);
		
		// send 'em to the outputs
		for (int i = 0; i < 8; i++) {
			outputs[SIGNAL_OUTPUTS + i].setVoltage((1.0f - params[MUTE_PARAMS + i].getValue()) * (i < 4 ? in1 : in2));
		}
	}
};


struct MuteIpleWidget : ModuleWidget {
MuteIpleWidget(MuteIple *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mute-iple.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, MuteIple::SIGNAL_INPUTS));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, MuteIple::SIGNAL_INPUTS + 1));
		
		// outputs/lights
		for (int i = 0; i < 8 ; i++) {
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::MUTE_PARAMS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, MuteIple::SIGNAL_OUTPUTS + i));	
		}
	}
};

Model *modelMuteIple = createModel<MuteIple, MuteIpleWidget>("Mute-iple");
