//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Sub Harmonic Generator
//	A sub harmonic generator  
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/Utility.hpp"

struct SubHarmonicGenerator : Module {
	enum ParamIds {
		ENUMS(MIX_PARAM, 5),
		ENUMS(DIV_PARAM, 4),
		OUTPUTLEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		OSC_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		NUM_OUTPUTS 
	};
	enum LightIds {
		OVERLOAD_LIGHT,
		NUM_LIGHTS
	};

	int divisions[5] = {1, 2, 4, 8, 16};
	FrequencyDivider dividers[5];
	
	bool antiAlias = false;
	
	SubHarmonicGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		char knobText[20];
		for (int i = 0; i < 5; i++) {
			sprintf (knobText, "Level %d", i + 1);
			configParam(MIX_PARAM + i, 0.0f, 1.0f, 0.0f, knobText, " %", 0.0f, 100.0f, 0.0f);

			if ( i < 4) {
				sprintf (knobText, "Division %d", i + 1);
				configParam(DIV_PARAM + i, 2.0f, 16.0f, (float)(divisions[i + 1]), knobText);
			}

			// dividers
			dividers[i].setN(divisions[i]);
		}

		// output level knobs
		configParam(OUTPUTLEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Output level", " %", 0.0f, 100.0f, 0.0f);
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		json_object_set_new(root, "antiAlias", json_boolean(antiAlias));
		
		return root;
	}
	
	void dataFromJson(json_t *root) override {
		
		json_t *aa = json_object_get(root, "antiAlias");

		if (aa)
			antiAlias = json_boolean_value(aa);
	}	

	void process(const ProcessArgs &args) override {
		float in = inputs[OSC_INPUT].getVoltage();	
		float out = 0.0f;

		// assemble the output value from all of the dividers
		for (int i = 0; i < 5 ; i++) {
			if (i > 0) {
				dividers[i].setMaxN(16);
				float div = (int)(params[DIV_PARAM + i -1].getValue());
				dividers[i].setN(div);
			}
			
			float x = boolToAudio(dividers[i].process(in));
			float y = params[MIX_PARAM + i].getValue();
			out += (x * y);
		}
	
		// apply the output level amount and set the overload indicator
		out = out * params[OUTPUTLEVEL_PARAM].getValue();
		float overloadLevel =  (fabs(out) > 11.2f) ? 1.0f : 0.0f;
		lights[OVERLOAD_LIGHT].setSmoothBrightness(overloadLevel, args.sampleTime);

		// set the output
		outputs[MIX_OUTPUT].setVoltage(clamp(out, -11.2f, 11.2f)); // TODO: saturate rather than clip
	}
};

struct SubHarmonicGeneratorWidget : ModuleWidget {
	SubHarmonicGeneratorWidget(SubHarmonicGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SubHarmonicGenerator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, SubHarmonicGenerator::OSC_INPUT));

		// harmonic level knobs
		for (int j = 0; j < 5; j++) {
			addParam(createParamCentered<CountModulaKnobViolet>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, SubHarmonicGenerator::MIX_PARAM + j));
			if (j < 4)
				addParam(createParamCentered<CountModulaRotarySwitchGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 +j]), module, SubHarmonicGenerator::DIV_PARAM + j));
		}
		
		// output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::MIX_OUTPUT));

		// overload light
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::OVERLOAD_LIGHT));
		
		// output level knob
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::OUTPUTLEVEL_PARAM));

	}
};

Model *modelSubHarmonicGenerator = createModel<SubHarmonicGenerator, SubHarmonicGeneratorWidget>("SubHarmonicGenerator");
