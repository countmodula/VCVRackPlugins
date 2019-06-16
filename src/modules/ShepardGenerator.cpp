//----------------------------------------------------------------------------
//	Count Modula - Shepard Generator Module
//	An 8 phase shepard oscillator.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct ShepardOscillator {
	float basePhases[8] = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };
	float phase[8] = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };
	float freq = 1.0f;

	dsp::SchmittTrigger resetTrigger;

	ShepardOscillator() {}
	
	void setPitch(float pitch) {
		pitch = fminf(pitch, 10.0f);
		freq = powf(2.0f, pitch);
	}
	
	void setReset(float reset) {
		if (resetTrigger.process(reset / 0.01f)) {
			for (int i = 0; i < 8; i++)
				phase[i] = basePhases[i];		
		}
	}
	
	void reset() {
		for (int i = 0; i < 8; i++)
			phase[i] = basePhases[i];

		resetTrigger.reset();
	}
	
	void step(float dt) {
		float deltaPhase = fminf(freq * dt, 0.5f);
		for (int i = 0; i < 8; i++) {
			phase[i] += deltaPhase;
			if (phase[i] >= 1.0f)
				phase[i] -= 1.0f;
		}
	}
	
	float saw(int i) {
		return 2.0f * (phase[i] - roundf(phase[i]));
	}
	
	float tri(int i) {
		return 4.0f * fabsf(phase[i] - roundf(phase[i]));
	}	
};

struct ShepardGenerator : Module {
	enum ParamIds {
		CV_PARAM,
		FREQ_PARAM,
		SAWLEVEL_PARAM,
		TRILEVEL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SAW_OUTPUT, 8),
		ENUMS(TRI_OUTPUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SHEP_LIGHT, 8),
		NUM_LIGHTS
	};

	
	ShepardOscillator osc; 
	
	ShepardGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "Frequency CV Level");
		configParam(FREQ_PARAM, -8.0f, 10.0f, -4.5f, "Frequency");
		configParam(SAWLEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Saw Output Level");
		configParam(TRILEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Triangle Output Level");
	}
	
	void onReset() override {
		osc.reset();
	}
	
	void process(const ProcessArgs &args) override {
		
		osc.setPitch(params[FREQ_PARAM].getValue() + (params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f)));
		osc.step(args.sampleTime);
		
		for (int i = 0; i < 8; i++) {
			outputs[SAW_OUTPUT + i].setVoltage((5.0f + (5.0f * osc.saw(i))) * params[SAWLEVEL_PARAM].getValue());
			float tri = 10.0f - (5.0f * osc.tri(i));
			outputs[TRI_OUTPUT + i].setVoltage(tri * params[TRILEVEL_PARAM].getValue());
			lights[SHEP_LIGHT + i].setBrightness(tri / 10.0f);
		}
	}
};

struct ShepardGeneratorWidget : ModuleWidget {
	ShepardGeneratorWidget(ShepardGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ShepardGenerator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, ShepardGenerator::CV_PARAM));
		addParam(createParamCentered<CountModulaKnobOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, ShepardGenerator::FREQ_PARAM));
		addParam(createParamCentered<CountModulaKnobGrey>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW5)), module, ShepardGenerator::SAWLEVEL_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, ShepardGenerator::TRILEVEL_PARAM));
		
		// cv input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW1)), module, ShepardGenerator::CV_INPUT));
		
		// outputs/lights
		for (int i = 0; i < 8 ; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::SAW_OUTPUT + i));	
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::SHEP_LIGHT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::TRI_OUTPUT + i));	
		}
	}
};

Model *modelShepardGenerator = createModel<ShepardGenerator, ShepardGeneratorWidget>("ShepardGenerator");
