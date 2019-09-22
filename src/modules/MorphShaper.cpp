//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Morph Shaper
//	A CV controlled morphing controller based on the Doepfer A-144
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MorphShaper
#define PANEL_FILE "MorphShaper.svg"

struct MorphShaper : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MORPH_OUTPUT, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(MORPH_LIGHT, 4),
		NUM_LIGHTS
	};

	float outVal;
	
#ifdef NOT_FULL_SCALE
	// thresholds for increasing output
	float pMap[4][2] = {
		{0.5f, 2.3f},
		{2.3f, 4.1f},
		{4.1f, 5.9f},
		{5.9f, 7.7f}
	};
	
	// thresholds for decreasing output
	float nMap[4][2] = {	
		{2.3f, 4.1f},
		{4.1f, 5.9f},
		{5.9f, 7.7f},
		{7.7f, 9.5f}
	};	
	
	const float span = 1.8f;
	const float scale = 10.0f / span;
	
#else
	// thresholds for increasing output
	float pMap[4][2] = {
		{0.0f, 2.0f},
		{2.0f, 4.0f},
		{4.0f, 6.0f},
		{6.0f, 8.0f}
	};
	
	// thresholds for decreasing output
	float nMap[4][2] = {	
		{2.0f, 4.0f},
		{4.0f, 6.0f},
		{6.0f, 8.0f},
		{8.0f, 10.0f}
	};	
	
	const float span = 2.0f;
	const float scale = 10.0f / span;	
#endif
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	MorphShaper() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// knobs
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "Morph CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(MANUAL_PARAM, 0.0f, 10.0f, 0.0f, "Manual morph");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
	
	void process(const ProcessArgs &args) override {
		
		// calculate the current morph control value
		float morphVal = clamp(params[MANUAL_PARAM].getValue() + (inputs[CV_INPUT].getVoltage() * params[CV_PARAM].getValue()), 0.0f, 10.0f);
			
		// reset all outputs
		for (int i = 0; i < 4; i++)
		{
			if (morphVal >= pMap[i][0] && morphVal < pMap[i][1])
				outVal = morphVal - pMap[i][0];
			else if (morphVal >= nMap[i][0] && morphVal < nMap[i][1])
				outVal = nMap[i][0] + span - morphVal;
			else
				outVal = 0.0f;
			
			outputs[MORPH_OUTPUT + i].setVoltage(outVal * scale);
			lights[MORPH_LIGHT + i].setBrightness(clamp (outVal, 0.0f, 1.0f));
		}
	}		
};

struct MorphShaperWidget : ModuleWidget {
	MorphShaperWidget(MorphShaper *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MorphShaper.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, MorphShaper::CV_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, MorphShaper::MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, MorphShaper::CV_INPUT));
		
		// outputs/lights
		for (int i = 0; i < 4 ; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + i]), module, MorphShaper::MORPH_OUTPUT +i));	
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + i]), module, MorphShaper::MORPH_LIGHT + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MorphShaper *module = dynamic_cast<MorphShaper*>(this->module);
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

Model *modelMorphShaper = createModel<MorphShaper, MorphShaperWidget>("MorphShaper");
