//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Attenuverter Module
//	A dual manual CV generator
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Attenuverter
#define PANEL_FILE "Attenuverter.svg"
#define ATTENUVERT 0
#define ATTENUATE 1

struct Attenuverter : Module {
	enum ParamIds {
		ATTENUATE_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(SIGNAL_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SIGNAL_OUTPUT, 2),
		ENUMS(INVERTED_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LEVEL_LIGHT, 2),
		NUM_LIGHTS
	};

	float outVal;
	int mode = ATTENUVERT;
	int prevMode = ATTENUVERT;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Attenuverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(ATTENUATE_PARAM, 0.0f, 1.0f, 0.0f, "Attenuation", " %", 0.0f, 100.0f, 0.0f);
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		mode = prevMode = ATTENUVERT;
		paramQuantities[Attenuverter::ATTENUATE_PARAM]->minValue = -1.0f;
	}		
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));

		json_object_set_new(root, "mode", json_integer(mode));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"

		json_t *m = json_object_get(root, "mode");

		if (m) {
			prevMode = json_integer_value(m);
			
			// adjust the min value based on the chosen mode mode
			paramQuantities[Attenuverter::ATTENUATE_PARAM]->minValue = (prevMode == ATTENUATE ? 0.0f : -1.0f);
		}
	}			
	
	void process(const ProcessArgs &args) override {
	
		float cv = params[ATTENUATE_PARAM].getValue();

		lights[LEVEL_LIGHT].setBrightness(-clamp(cv, -1.0f, 0.0f));
		lights[LEVEL_LIGHT + 1].setBrightness(clamp(cv, 0.0f, 1.0f));
		
		mode = params[MODE_PARAM].getValue() > 0.5f ? ATTENUATE : ATTENUVERT;
		if (prevMode != mode) {
			// adjust the min value based on the chosen mode mode
			paramQuantities[Attenuverter::ATTENUATE_PARAM]->minValue = (mode == ATTENUATE ? 0.0f : -1.0f);

			// on change of mode, adjust the control value so it stays in the same position
			switch (mode) {
				case ATTENUVERT:
					params[ATTENUATE_PARAM].setValue((cv/1.0f * 2.0f) - 1.0f);
					break;
				case ATTENUATE:
					params[ATTENUATE_PARAM].setValue((cv + 1.0f) / 2.0f);
					break;
			}
		}

		prevMode = mode;
		
		for (int i = 0; i < 2; i ++) {
		
			if (inputs[SIGNAL_INPUT + i].isConnected()) {
				int n = inputs[SIGNAL_INPUT + i].getChannels();
			
				outputs[SIGNAL_OUTPUT + i].setChannels(n);
				outputs[INVERTED_OUTPUT + i].setChannels(n);
				
				for (int c =- 0; c < n; c++) {
					float v = inputs[SIGNAL_INPUT + i].getVoltage(c);
					outputs[SIGNAL_OUTPUT + i].setVoltage(v * cv, c);
					outputs[INVERTED_OUTPUT + i].setVoltage(-v * cv, c);
				}
			}
			else
				outputs[SIGNAL_OUTPUT + i].channels = 0;
		}
	}
};

struct AttenuverterWidget : ModuleWidget {
	AttenuverterWidget(Attenuverter *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Attenuverter.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<CountModulaKnobMegaYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_HALF_ROWS7(STD_ROW6)), module, Attenuverter::ATTENUATE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Attenuverter::MODE_PARAM));

		// lights
		addChild(createLightCentered<MediumLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Attenuverter::LEVEL_LIGHT));
		
		
		for (int i = 0; i < 2; i ++) {
			// inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + (i * 2)], STD_ROWS6[STD_ROW1]), module, Attenuverter::SIGNAL_INPUT + i));

			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + (i * 2)], STD_ROWS6[STD_ROW2]), module, Attenuverter::SIGNAL_OUTPUT + i));	
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + (i * 2)], STD_ROWS6[STD_ROW3]), module, Attenuverter::INVERTED_OUTPUT + i));
			
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Attenuverter *module = dynamic_cast<Attenuverter*>(this->module);
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

Model *modelAttenuverter = createModel<Attenuverter, AttenuverterWidget>("Attenuverter");
