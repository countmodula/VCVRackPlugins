//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Shepard Generator Module
//	An 8 phase shepard oscillator.
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ShepardGenerator
#define PANEL_FILE "ShepardGenerator.svg"

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
		PSAW_OUTPUT,
		PTRI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SHEP_LIGHT, 8),
		NUM_LIGHTS
	};

	ShepardOscillator osc; 

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	ShepardGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "Frequency CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(FREQ_PARAM, -8.0f, 10.0f, -4.5f, "Frequency");
		configParam(SAWLEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Saw output level", " %", 0.0f, 100.0f, 0.0f);
		configParam(TRILEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Triangle output level", " %", 0.0f, 100.0f, 0.0f);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.1"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"			
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		

	void onReset() override {
		osc.reset();
	}
	
	void process(const ProcessArgs &args) override {
		
		osc.setPitch(params[FREQ_PARAM].getValue() + (params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f)));
		osc.step(args.sampleTime);
		
		outputs[PSAW_OUTPUT].setChannels(8);
		outputs[PTRI_OUTPUT].setChannels(8);

		for (int i = 0; i < 8; i++) {
			float saw = (5.0f + (5.0f * osc.saw(i))) * params[SAWLEVEL_PARAM].getValue();
			float tri = 10.0f - (5.0f * osc.tri(i));
			lights[SHEP_LIGHT + i].setBrightness(tri / 10.0f);
			tri = tri * params[TRILEVEL_PARAM].getValue();
			outputs[SAW_OUTPUT + i].setVoltage(saw);
			outputs[TRI_OUTPUT + i].setVoltage(tri);
			
			outputs[PSAW_OUTPUT].setVoltage(saw, i);
			outputs[PTRI_OUTPUT].setVoltage(tri, i);
		}
	}
};

struct ShepardGeneratorWidget : ModuleWidget {
	ShepardGeneratorWidget(ShepardGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ShepardGenerator.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, ShepardGenerator::CV_PARAM));
		addParam(createParamCentered<CountModulaKnobOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW3) - 5), module, ShepardGenerator::FREQ_PARAM));
		addParam(createParamCentered<CountModulaKnobGrey>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5] -10 ), module, ShepardGenerator::SAWLEVEL_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW6) - 15), module, ShepardGenerator::TRILEVEL_PARAM));
		
		// cv input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, ShepardGenerator::CV_INPUT));
		
		// outputs/lights
		for (int i = 0; i < 8 ; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::SAW_OUTPUT + i));	
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::SHEP_LIGHT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, ShepardGenerator::TRI_OUTPUT + i));	
		}
		
		// poly outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1]-15, STD_ROWS8[STD_ROW8]), module, ShepardGenerator::PSAW_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2]-15, STD_ROWS8[STD_ROW8]), module, ShepardGenerator::PTRI_OUTPUT));	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ShepardGenerator *module = dynamic_cast<ShepardGenerator*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};

Model *modelShepardGenerator = createModel<ShepardGenerator, ShepardGeneratorWidget>("ShepardGenerator");
