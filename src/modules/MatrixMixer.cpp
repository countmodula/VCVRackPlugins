//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Matrix Mixer Module
//	A 4 x 4 matrix mixer with switchable uni/bi polar mixing capabilities
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/MixerEngine.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MatrixMixer
#define PANEL_FILE "MatrixMixer.svg"

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

	int processCount = 8;
	
	float mixLevels1[4] = {};
	float mixLevels2[4] = {};
	float mixLevels3[4] = {};
	float mixLevels4[4] = {};
	float outputLevels[4] = {};
	bool bipolarMode[4] = {};
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	MatrixMixer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char smeg[10];
		char c = 'A';
		for (int i = 0; i < 4; i++) {
			
			sprintf (smeg, "Level %d", i + 1);
			
			// mix knobs
			for (int j = 0; j < 4; j++)
				configParam(C1R1_LEVEL_PARAM + (j * 6) + i, 0.0f, 1.0f, 0.5f, string::f("Input %c mix %d level", c, j + 1), " %", 0.0f, 100.0f, 0.0f);
			
			// output level knobs
			configParam(C1_LEVEL_PARAM + (i * 6), 0.0f, 1.0f, 0.0f, string::f("Mix %d output level", i + 1), " %", 0.0f, 100.0f, 0.0f);
			
			// mode switches
			configSwitch(C1_MODE_PARAM + (i * 6), 0.0f, 1.0f, 1.0f, string::f("Mix %d mode", i + 1), {"Unipolar", "Bipolar"});
			
			// ins and outs
			configOutput(C1_OUTPUT + i,  string::f("Mix %d", i + 1));
			configInput(R1_INPUT +i, string::f("%c", c++));
		}

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		processCount = 8;
	}
	
	void onReset() override {
		processCount = 8;
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		

		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void process(const ProcessArgs &args) override {
		
		if (++processCount > 0) {
			processCount = 0;
			
			for (int i = 0; i < 4; i++) {
				mixLevels1[i] = params[C1R1_LEVEL_PARAM + (i * 6)].getValue();
				mixLevels2[i] = params[C1R2_LEVEL_PARAM + (i * 6)].getValue();
				mixLevels3[i] = params[C1R3_LEVEL_PARAM + (i * 6)].getValue();
				mixLevels4[i] = params[C1R4_LEVEL_PARAM + (i * 6)].getValue();
				outputLevels[i] = params[C1_LEVEL_PARAM + (i * 6)].getValue();
				bipolarMode[i] = params[C1_MODE_PARAM + (i * 6)].getValue() > 0.5f;
			}
		}
		
		float st = args.sampleTime * 4.0f;
		for (int i = 0; i < 4; i++) {
			outputs[C1_OUTPUT + i].setVoltage(mixers[i].process(inputs[R1_INPUT].getNormalVoltage(0.0f), inputs[R2_INPUT].getNormalVoltage(0.0f), inputs[R3_INPUT].getNormalVoltage(0.0f), inputs[R4_INPUT].getNormalVoltage(0.0f), 
				mixLevels1[i], mixLevels2[i], mixLevels3[i], mixLevels4[i], 
				outputLevels[i], bipolarMode[i]));
			
			if (processCount == 0)
				lights[C1_OVERLOAD_LIGHT  + i].setSmoothBrightness(mixers[i].overloadLevel, st);
		}
	}
};

struct MatrixMixerWidget : ModuleWidget {

	std::string panelName;
	
	MatrixMixerWidget(MatrixMixer *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		for (int i = 0; i < 4; i++) {
			// mix `knobs
			for (int j = 0; j < 4; j++) {
				switch (j) {
					case 0:
						addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 1:
						addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 2:
						addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
					case 3:
						addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[(j + 1) * 2], STD_ROWS6[i]), module, MatrixMixer::C1R1_LEVEL_PARAM + (j * 6) + i));
						break;
				}
			}
			
			// level knobs
			addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[(i + 1) * 2], STD_ROWS6[STD_ROW5]), module, MatrixMixer::C1_LEVEL_PARAM + (i * 6)));
			
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
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MatrixMixer *module = dynamic_cast<MatrixMixer*>(this->module);
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

Model *modelMatrixMixer = createModel<MatrixMixer, MatrixMixerWidget>("MatrixMixer");
