//----------------------------------------------------------------------------
//	Count Modula - Mixer Module
//	A 4 input mixer with switchable uni/bi polar mixing capabilities
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/MixerEngine.hpp"

struct Mixer : Module {
	enum ParamIds {
		R1_LEVEL_PARAM,
		R2_LEVEL_PARAM,
		R3_LEVEL_PARAM,
		R4_LEVEL_PARAM,
		MODE_PARAM,
		LEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		R1_INPUT,
		R2_INPUT,
		R3_INPUT,
		R4_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		XIM_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		OVERLOAD_LIGHT,
		NUM_LIGHTS
	};

	MixerEngine mixer;
	
	Mixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char smeg[10];
		
		// mix `knobs
		for (int i = 0; i < 4; i++) {
			sprintf (smeg, "Level %d", i + 1);
			configParam(R1_LEVEL_PARAM + i, 0.0f, 1.0f, 0.5f, smeg);
		}
		
		// level knobs
		configParam(LEVEL_PARAM, 0.0f, 1.0f, 0.0f, "Output Level");
		
		// switches
		configParam(MODE_PARAM, 0.0f, 1.0f, 1.0f, "Mix Mode (Uni/Bipolar)");
	}
	
	void process(const ProcessArgs &args) override {
		float out = mixer.process(inputs[R1_INPUT].getNormalVoltage(10.0f), inputs[R2_INPUT].getNormalVoltage(0.0f), inputs[R3_INPUT].getNormalVoltage(0.0f), inputs[R4_INPUT].getNormalVoltage(0.0f), 
			params[R1_LEVEL_PARAM].getValue(), params[R2_LEVEL_PARAM].getValue(), params[R3_LEVEL_PARAM].getValue(), params[R4_LEVEL_PARAM].getValue(), 
			params[LEVEL_PARAM].getValue(), params[MODE_PARAM].getValue());
			
		outputs[MIX_OUTPUT].setVoltage(out);
		outputs[XIM_OUTPUT].setVoltage(-out);
		
		lights[OVERLOAD_LIGHT].setSmoothBrightness(mixer.overloadLevel, args.sampleTime);
	}
};

struct MixerWidget : ModuleWidget {
	MixerWidget(Mixer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mixer.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int j = 0; j < 4; j++) {
			addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, Mixer::R1_LEVEL_PARAM + j));
			
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, Mixer::R1_INPUT + j));
		}
			
		// level knobs
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Mixer::LEVEL_PARAM));
		

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Mixer::XIM_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Mixer::MIX_OUTPUT));

		// switches
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, Mixer::MODE_PARAM));

		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5]), module, Mixer::OVERLOAD_LIGHT));
	}
};

Model *modelMixer = createModel<Mixer, MixerWidget>("Mixer");
