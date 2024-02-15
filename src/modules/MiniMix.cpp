//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - MiniMix Module
//	A 4 input mini mixer with switchable uni/bi polar mixing capabilities
//	Copyright (C) 2024  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/MixerEngine.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MiniMix
#define PANEL_FILE "MiniMix.svg"

using simd::float_4;

struct MiniMix : Module {
	enum ParamIds {
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
		POLY_OVERLOAD_LIGHT,
		NUM_LIGHTS
	};

	MixerEngine mixer;

	float outputLevel = 0.0f;
	bool limitToRails = true;
	
	int processCount = 8;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	MiniMix() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// inputs
		char c ='A';
		std::string s;
		for (int i = 0; i < 4; i++) {
			s = c++;
			configInput(R1_INPUT + i, s);
		}

		// output level 
		configParam(LEVEL_PARAM, -1.0f, 1.0f, 0.0f, "Output level", " %", 0.0f, 100.0f, 0.0f);
		
		// outputs
		configOutput(MIX_OUTPUT, "Mix");
		configOutput(XIM_OUTPUT, "Inverted mix");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		processCount = 8;
	}	
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "clipping", json_boolean(limitToRails));	
	
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		limitToRails = true;
		json_t *clip = json_object_get(root, "clipping");		
		if (clip)
			limitToRails = json_boolean_value(clip);
	}
	
	void process(const ProcessArgs &args) override {
		
		if (++processCount > 8) {
			processCount = 0;
			outputLevel = params[LEVEL_PARAM].getValue();
		}
		
		int numChannels = 1;
		numChannels = std::max(numChannels, inputs[R1_INPUT].getChannels());
		numChannels = std::max(numChannels, inputs[R2_INPUT].getChannels());
		numChannels = std::max(numChannels, inputs[R3_INPUT].getChannels());
		numChannels = std::max(numChannels, inputs[R4_INPUT].getChannels());
	
		outputs[MIX_OUTPUT].setChannels(numChannels);
		outputs[XIM_OUTPUT].setChannels(numChannels);
				
		float overloadLevel = 0.0f;
		for (int c = 0; c < numChannels; c += 4) {
			float_4 out = 0.f;
			
			out += inputs[R1_INPUT].getVoltageSimd<float_4>(c);
			out += inputs[R2_INPUT].getVoltageSimd<float_4>(c);
			out += inputs[R3_INPUT].getVoltageSimd<float_4>(c);
			out += inputs[R4_INPUT].getVoltageSimd<float_4>(c);

			out *= outputLevel;

			for (int i = 0; i < 4; i++) {
				if (fabs(out[i]) > 12.0f) {
					overloadLevel = 1.0;
					break;
				}
			}
			
			// TODO: saturation rather than hard limit although my hardware mixers just clip
			if (limitToRails) {
				out = clamp(out, -12.0f, 12.0f);
			}

			outputs[MIX_OUTPUT].setVoltageSimd(out, c);
			outputs[XIM_OUTPUT].setVoltageSimd(-out, c);
		}		
		
		if (processCount == 0) {
			if (numChannels > 1) {
				lights[OVERLOAD_LIGHT].setSmoothBrightness(0.0f, args.sampleTime * 4);
				lights[POLY_OVERLOAD_LIGHT].setSmoothBrightness(overloadLevel, args.sampleTime * 4);
			}
			else {
				lights[OVERLOAD_LIGHT].setSmoothBrightness(overloadLevel, args.sampleTime * 4);
				lights[POLY_OVERLOAD_LIGHT].setSmoothBrightness(0.0f, args.sampleTime * 4);
			}
		}
	}
};

struct MiniMixWidget : ModuleWidget {

	std::string panelName;
	
	MiniMixWidget(MiniMix *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		// inputs
		for (int j = 0; j < 4; j++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1 + j]), module, MiniMix::R1_INPUT + j));
		}
		
		// controls
		addParam(createParamCentered<Potentiometer<SmallKnob<WhiteKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, MiniMix::LEVEL_PARAM));
		
		// output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, MiniMix::MIX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, MiniMix::XIM_OUTPUT));

		// lights
		addChild(createLightCentered<MediumLight<CountModulaLightRB>>(Vec(STD_COLUMN_POSITIONS[STD_COL1]+22, STD_ROWS7[STD_ROW5]-19), module, MiniMix::OVERLOAD_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MiniMix *module = dynamic_cast<MiniMix*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));		
		
		menu->addChild(createBoolPtrMenuItem("Clipping", "", &module->limitToRails));		
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}
};

Model *modelMiniMix = createModel<MiniMix, MiniMixWidget>("MiniMix");
