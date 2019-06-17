//----------------------------------------------------------------------------
//	Count Modula - Matrix Mixer Module
//	A 4 x 4 matrix mixer with switchable uni/bi polar mixing capabilities
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/MixerEngine.hpp"

struct MatrixMixer : Module {
	enum ParamIds {
		C1R1_LEVEL_PARAM,
		C1R2_LEVEL_PARAM,
		C1R3_LEVEL_PARAM,
		C1R4_LEVEL_PARAM,
		C1_MODE_PARAM,
		C1_LEVEL_PARAM,
		C2R1_LEVEL_PARAM,
		C2R2_LEVEL_PARAM,
		C2R3_LEVEL_PARAM,
		C2R4_LEVEL_PARAM,
		C2_MODE_PARAM,
		C2_LEVEL_PARAM,
		C3R1_LEVEL_PARAM,
		C3R2_LEVEL_PARAM,
		C3R3_LEVEL_PARAM,
		C3R4_LEVEL_PARAM,
		C3_MODE_PARAM,
		C3_LEVEL_PARAM,
		C4R1_LEVEL_PARAM,
		C4R2_LEVEL_PARAM,
		C4R3_LEVEL_PARAM,
		C4R4_LEVEL_PARAM,
		C4_MODE_PARAM,
		C4_LEVEL_PARAM,
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
		C1_OUTPUT,
		C2_OUTPUT,
		C3_OUTPUT,
		C4_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		C1_OVERLOAD_LIGHT,
		C2_OVERLOAD_LIGHT,
		C3_OVERLOAD_LIGHT,
		C4_OVERLOAD_LIGHT,
		NUM_LIGHTS
	};

	MixerEngine mixers[4];
	
	MatrixMixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char smeg[10];
		
		
		for (int i = 0; i < 4; i++) {
			
			sprintf (smeg, "Level %d", i + 1);
			
			// mix `knobs
			for (int j = 0; j < 4; j++)
				configParam(C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f, smeg, " %", 0.0f, 100.0f, 0.0f);
			
			// level knobs
			configParam(C1_LEVEL_PARAM + (i * 6), 0.0f, 1.0f, 0.0f, "Output level", " %", 0.0f, 100.0f, 0.0f);
			
			// switches
			configParam(C1_MODE_PARAM + (i * 6), 0.0f, 1.0f, 1.0f, "Channel mode (Uni/Bipolar)");
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		for (int i = 0; i < 4; i++) {
			outputs[C1_OUTPUT + i].setVoltage(mixers[i].process(inputs[R1_INPUT].getNormalVoltage(0.0f), inputs[R2_INPUT].getNormalVoltage(0.0f), inputs[R3_INPUT].getNormalVoltage(0.0f), inputs[R4_INPUT].getNormalVoltage(0.0f), 
				params[C1R1_LEVEL_PARAM + (i * 6)].getValue(), params[C1R2_LEVEL_PARAM + (i * 6)].getValue(), params[C1R3_LEVEL_PARAM + (i * 6)].getValue(), params[C1R4_LEVEL_PARAM + (i * 6)].getValue(), 
				params[C1_LEVEL_PARAM + (i * 6)].getValue(), params[C1_MODE_PARAM + (i * 6)].getValue()));
				
			lights[C1_OVERLOAD_LIGHT  + i].setSmoothBrightness(mixers[i].overloadLevel, args.sampleTime);
		}
	}
};

struct MatrixMixerWidget : ModuleWidget {
	MatrixMixerWidget(MatrixMixer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MatrixMixer.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int i = 0; i < 4; i++) {
			// mix `knobs
			for (int j = 0; j < 4; j++) {
				switch (j) {
					case 0:
						addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 1:
						addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 2:
						addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 3:
						addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
				}
			}
			
			// level knobs
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2], STD_ROWS6[STD_ROW5]), module, MatrixMixer::C1_LEVEL_PARAM + (i * 6)));
			
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[i]), module, MatrixMixer::R1_INPUT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2] + 15, STD_ROWS6[STD_ROW6] + 5), module, MatrixMixer::C1_OUTPUT + i));

			// switches
			addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[(i * 2) + 1] + 15, STD_ROWS6[STD_ROW6] + 5), module, MatrixMixer::C1_MODE_PARAM + (i * 6)));

			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2] + 15, STD_ROWS6[STD_ROW6] - 20), module, MatrixMixer::C1_OVERLOAD_LIGHT + i));
		}
	}
};

Model *modelMatrixMixer = createModel<MatrixMixer, MatrixMixerWidget>("MatrixMixer");
