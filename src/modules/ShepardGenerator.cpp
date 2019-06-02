//----------------------------------------------------------------------------
//	Count Modula - Shepard Generator Module
//	An 8 phase shepard oscillator.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "dsp/digital.hpp"

struct ShepardOscillator {
	float basePhases[8] = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };
	float phase[8] = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f };
	float freq = 1.0f;

	SchmittTrigger resetTrigger;

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
	ShepardGenerator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void onReset() override {
		osc.reset();
	}
	
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void ShepardGenerator::step() {
	
	osc.setPitch(params[FREQ_PARAM].value + (params[CV_PARAM].value * inputs[CV_INPUT].value));
	osc.step(engineGetSampleTime());
	
	for (int i = 0; i < 8; i++) {
		outputs[SAW_OUTPUT + i].value = (5.0f + (5.0f * osc.saw(i))) * params[SAWLEVEL_PARAM].value;
		float tri = 10.0f - (5.0f * osc.tri(i));
		outputs[TRI_OUTPUT + i].value = tri * params[TRILEVEL_PARAM].value;
		lights[SHEP_LIGHT + i].setBrightnessSmooth(tri / 10.0f);
	}
}

struct ShepardGeneratorWidget : ModuleWidget {
ShepardGeneratorWidget(ShepardGenerator *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/ShepardGenerator.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, ShepardGenerator::CV_PARAM, -1.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, ShepardGenerator::FREQ_PARAM, -8.0f, 10.0f, -4.5f));
		addParam(createParamCentered<CountModulaKnobGrey>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW5)), module, ShepardGenerator::SAWLEVEL_PARAM, 0.0f, 1.0f, 1.0f));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, ShepardGenerator::TRILEVEL_PARAM, 0.0f, 1.0f, 1.0f));
		
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

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelShepardGenerator = Model::create<ShepardGenerator, ShepardGeneratorWidget>("Count Modula", "ShepardGenerator", "Shepard Generator", LFO_TAG);
