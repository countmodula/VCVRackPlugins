//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 1 - 8 random access switch
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME RandomAccessSwitch18
#define PANEL_FILE "RandomAccessSwitch18.svg"

struct RandomAccessSwitch18 : Module {
	enum ParamIds {
		ENUMS(SELECT_PARAM, 8),
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		ENUMS(SELECT_INPUT, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CV_OUTPUT, 8),
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECT_LIGHT, 8),
		TRIG_LIGHT,
		NUM_LIGHTS
	};

	enum Modes {
		MODE_TRACK_AND_HOLD,
		MODE_PASS_THROUGH,
		MODE_SAMPLE_AND_HOLD
	};
	
	GateProcessor gpSelect[8];
	dsp::PulseGenerator pgChange;
	int selection = 0, prevSelection = 0;
	int processCount = 0;
	int mode = 1;
	float presetVoltages[8] = {};
	bool preset = false;
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	RandomAccessSwitch18() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configSwitch(MODE_PARAM, 0.0, 2.0, 1.0, "Mode", {"Track and hold", "Pass though", "Sample and hold"});
		
		configInput(CV_INPUT, "Source");
		configOutput(TRIG_OUTPUT, "Change trigger");

		for (int i = 0, c = 1; i < 8; i++, c++) {
			configButton(SELECT_PARAM + i, rack::string::f("Select %d", c));
			configInput(SELECT_INPUT + i, rack::string::f("Select %d", c));
			configOutput(CV_OUTPUT + i, rack::string::f("Destination %d", c));
			presetVoltages[i] = 0.0f;
		}
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"	
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "selection", json_integer(selection));
		json_object_set_new(root, "mode", json_integer(mode));
		
		// save the current output voltages
		json_t *v = json_array();
		for (int i = 0; i < 8 ; i++) {
			json_array_insert_new(v, i, json_real(outputs[CV_OUTPUT + i].getVoltage()));
		}
		json_object_set_new(root, "outputVoltages", v);

		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		
		json_t *sel = json_object_get(root, "selection");
		json_t *mod = json_object_get(root, "mode");
		json_t *vl = json_object_get(root, "outputVoltages");

		if (sel)
			selection = json_integer_value(sel);	

		if (mod)
			mode = json_integer_value(mod);	
			
		// read the saved output voltages
		if (vl) {
			preset = true;
			for (int i = 0; i < 8 ; i++) {
				json_t *v = json_array_get(vl, i);
				if (v) {
					presetVoltages[i] = json_real_value(v);
				}
			}
		}

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}

	void onReset() override {
		prevSelection = selection;
		selection = 0;
		pgChange.reset();
	}
	
	void process(const ProcessArgs &args) override {
	
		// set the output voltages to what they when the patch was saved.
		if (preset) {
			DEBUG("Setting preset voltages");
			for (int i = 0; i < 8 ; i++) {
				outputs[CV_OUTPUT + i].setVoltage(presetVoltages[i]);
			}
			preset = false;
		}

		bool sample = false;
		if (++ processCount > 8) {
			
			mode = (int)(params[MODE_PARAM].getValue());
			
			for (int i = 0; i < 8; i++) {
				gpSelect[i].set(params[SELECT_PARAM + i].getValue() > 0.5f ? 10.0f : inputs[SELECT_INPUT + i].getVoltage());
				if (gpSelect[i].leadingEdge()) {
					selection = i;
					sample = true;
				}
			}
			
			processCount = 0;
		}
		
		for (int i = 0; i < 8; i++) {
			if (i == selection) {
				lights[SELECT_LIGHT + i].setBrightness(1.0f);
				if (mode != MODE_SAMPLE_AND_HOLD || (mode == MODE_SAMPLE_AND_HOLD && sample))
					outputs[CV_OUTPUT + i].setVoltage(inputs[CV_INPUT].getVoltage());
			}
			else
			{
				lights[SELECT_LIGHT + i].setBrightness(0.0f);
				if (mode == MODE_PASS_THROUGH)
					outputs[CV_OUTPUT + i].setVoltage(0.0);
			}
		}
		
		if (prevSelection != selection || (mode == MODE_SAMPLE_AND_HOLD && sample)) {
			pgChange.trigger(1e-3f);
			prevSelection = selection;
		}
		
		if (pgChange.remaining > 0.0f) {
			outputs[TRIG_OUTPUT].setVoltage(10.0f);
			lights[TRIG_LIGHT].setBrightness(1.0);
		}
		else {
			outputs[TRIG_OUTPUT].setVoltage(0.0f);
			lights[TRIG_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		pgChange.process(args.sampleTime);

	}
};

struct RandomAccessSwitch18Widget : ModuleWidget {
	std::string panelName;
	
	// custom columns for this module to squeeze it up a little
	const int COLUMN_POSITIONS[8] = {
		30,
		55,
		80,
		105,
		130,
		155,
		180,
		205
	};

	RandomAccessSwitch18Widget(RandomAccessSwitch18 *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW4)), module, RandomAccessSwitch18::CV_INPUT));
		
		addParam(createParamCentered<CountModulaToggle3P>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, RandomAccessSwitch18::MODE_PARAM));

		addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, RandomAccessSwitch18::TRIG_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS8[STD_ROW7] - 19), module, RandomAccessSwitch18::TRIG_LIGHT));

		for (int i = 0; i < 8; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL3], STD_ROWS8[i]), module, RandomAccessSwitch18::SELECT_INPUT + i));
			addParam(createParamCentered<CountModulaUnlitPushButtonMomentary>(Vec(COLUMN_POSITIONS[STD_COL5], STD_ROWS8[i]), module, RandomAccessSwitch18::SELECT_PARAM + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(COLUMN_POSITIONS[STD_COL7], STD_ROWS8[i]), module, RandomAccessSwitch18::CV_OUTPUT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(COLUMN_POSITIONS[STD_COL6], STD_ROWS8[i]), module, RandomAccessSwitch18::SELECT_LIGHT + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		RandomAccessSwitch18 *module = dynamic_cast<RandomAccessSwitch18*>(this->module);
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


Model *modelRandomAccessSwitch18 = createModel<RandomAccessSwitch18, RandomAccessSwitch18Widget>("RandomAccessSwitch18");
