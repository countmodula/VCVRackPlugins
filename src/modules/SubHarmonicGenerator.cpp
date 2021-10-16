//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Sub Harmonic Generator
//	A sub harmonic generator  
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SubHarmonicGenerator
#define PANEL_FILE "SubHarmonicGenerator.svg"

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

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SubHarmonicGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		char cm = 'A';
		char cd = 'A';
		for (int i = 0; i < 5; i++) {
			if (i == 0)
				configParam(MIX_PARAM + i, 0.0f, 1.0f, 0.0f, "Divide by 1 mix level", " %", 0.0f, 100.0f, 0.0f);
			else
				configParam(MIX_PARAM + i, 0.0f, 1.0f, 0.0f, string::f("Divide by %c mix level", cm++), " %", 0.0f, 100.0f, 0.0f);

			if ( i < 4) {
				configParam(DIV_PARAM + i, 2.0f, 16.0f, (float)(divisions[i + 1]), string::f("Division %c", cd++));
			}

			// dividers
			dividers[i].setN(divisions[i]);
		}

		// output level knobs
		configParam(OUTPUTLEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Output level", " %", 0.0f, 100.0f, 0.0f);

		configInput(OSC_INPUT, "Oscillator");
		configOutput(MIX_OUTPUT, "Sub-harmonic mix");
			
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "antiAlias", json_boolean(antiAlias));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t *root) override {
		
		json_t *aa = json_object_get(root, "antiAlias");

		if (aa)
			antiAlias = json_boolean_value(aa);
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
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

	std::string panelName;
	
	SubHarmonicGeneratorWidget(SubHarmonicGenerator *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, SubHarmonicGenerator::OSC_INPUT));

		// harmonic level knobs
		for (int j = 0; j < 5; j++) {
			addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, SubHarmonicGenerator::MIX_PARAM + j));
			if (j < 4)
				addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 +j]), module, SubHarmonicGenerator::DIV_PARAM + j));
		}
		
		// output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::MIX_OUTPUT));

		// overload light
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::OVERLOAD_LIGHT));
		
		// output level knob
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, SubHarmonicGenerator::OUTPUTLEVEL_PARAM));

	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"	
	
	void appendContextMenu(Menu *menu) override {
		SubHarmonicGenerator *module = dynamic_cast<SubHarmonicGenerator*>(this->module);
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

Model *modelSubHarmonicGenerator = createModel<SubHarmonicGenerator, SubHarmonicGeneratorWidget>("SubHarmonicGenerator");
