//----------------------------------------------------------------------------
//	Count Modula - Matrix Mixer Module
//	A 4 x 4 matrix mixer with switchable uni/bi polar mixing capabilities
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct MixerEngine {
	float overloadLevel = 0.0f;
	float mixLevel = 0.0f;

	float process (float input1, float input2, float input3, float input4, float inputLevel1, float inputLevel2, float inputLevel3, float inputLevel4, float outputLevel, float mode) {
	
		float level1 = inputLevel1;
		float level2 = inputLevel2;
		float level3 = inputLevel3;
		float level4 = inputLevel4;
		
		// convert to unipolar
		if (mode > 0.5) {
			level1 = (clamp(inputLevel1, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level2 = (clamp(inputLevel2, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level3 = (clamp(inputLevel3, 0.0f, 1.0f) * 2.0f) - 1.0f;
			level4 = (clamp(inputLevel4, 0.0f, 1.0f) * 2.0f) - 1.0f;
		}
	
		float out = outputLevel * ((input1 * level1) +
									(input2 * level2) +
									(input3 * level3) +
									(input4 * level4));
		
		overloadLevel =  (fabs(out) > 10.0f) ? 1.0f : 0.0f;
		mixLevel = fminf(fabs(out) / 10.0f, 1.0f);

		return out;
	}
};


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
	
	MatrixMixer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void MatrixMixer::step() {
	
	for (int i = 0; i < 4; i++) {
		outputs[C1_OUTPUT + i].value = mixers[i].process(inputs[R1_INPUT].normalize(0.0f), inputs[R2_INPUT].normalize(0.0f), inputs[R3_INPUT].normalize(0.0f), inputs[R4_INPUT].normalize(0.0f), 
			params[C1R1_LEVEL_PARAM + (i * 6)].value, params[C1R2_LEVEL_PARAM + (i * 6)].value, params[C1R3_LEVEL_PARAM + (i * 6)].value, params[C1R4_LEVEL_PARAM + (i * 6)].value, 
			params[C1_LEVEL_PARAM + (i * 6)].value, params[C1_MODE_PARAM + (i * 6)].value);
			
		lights[C1_OVERLOAD_LIGHT  + i].setBrightnessSmooth(mixers[i].overloadLevel);
	}
}

struct MatrixMixerWidget : ModuleWidget {
	MatrixMixerWidget(MatrixMixer *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MatrixMixer.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int i = 0; i < 4; i++) {
			// mix `knobs
			for (int j = 0; j < 4; j++) {
				switch (j) {
					case 0:
						addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f));
						break;
					case 1:
						addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f));
						break;
					case 2:
						addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f));
						break;
					case 3:
						addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f));
						break;
				}
			}
			
			// level knobs
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2], STD_ROWS6[STD_ROW5]), module, MatrixMixer::C1_LEVEL_PARAM + (i * 6), 0.0f, 1.0f, 0.0f));
			
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[i]), module, MatrixMixer::R1_INPUT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2] + 15, STD_ROWS6[STD_ROW6] + 5), module, MatrixMixer::C1_OUTPUT + i));

			// switches
			addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[(i * 2) + 1] + 15, STD_ROWS6[STD_ROW6] + 5), module, MatrixMixer::C1_MODE_PARAM + (i * 6), 0.0f, 1.0f, 1.0f));

			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2] + 15, STD_ROWS6[STD_ROW6] - 20), module, MatrixMixer::C1_OVERLOAD_LIGHT + i));
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMatrixMixer = Model::create<MatrixMixer, MatrixMixerWidget>("Count Modula", "MatrixMixer", "Matrix Mixer", MIXER_TAG);
