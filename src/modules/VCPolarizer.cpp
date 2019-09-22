//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Polarizer Module
//	AA 2 channel voltage controlled signal polarizer
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME VCPolarizer
#define PANEL_FILE "VCPolarizer.svg"

struct VCPolarizer : Module {
	enum ParamIds {
		CH1_CVAMOUNT_PARAM,
		CH1_MANUAL_PARAM,
		CH2_CVAMOUNT_PARAM,
		CH2_MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_CV_INPUT,
		CH1_SIGNAL_INPUT,
		CH2_CV_INPUT,
		CH2_SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH1_SIGNAL_OUTPUT,
		CH2_SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CH1_POS_LIGHT,
		CH1_NEG_LIGHT,
		CH2_POS_LIGHT,
		CH2_NEG_LIGHT,
		NUM_LIGHTS
	};

	Polarizer polarizer1;
	Polarizer polarizer2;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	VCPolarizer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CH1_CVAMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "CV Amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(CH1_MANUAL_PARAM, -2.0f, 2.0f, 0.0f, "Manual Amount");
		configParam(CH2_CVAMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "CV Amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(CH2_MANUAL_PARAM, -2.0f, 2.0f, 0.0f, "Manual Amount");

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
	
	void onReset() override {
		polarizer1.reset();
		polarizer2.reset();
	}	
	
	void process(const ProcessArgs &args) override {

		// channel 1
		
		float manual = params[CH1_MANUAL_PARAM].getValue();
		float cv = inputs[CH1_CV_INPUT].getVoltage();
		float cvAmount = params[CH1_CVAMOUNT_PARAM].getValue();

		int n = inputs[CH1_SIGNAL_INPUT].getChannels();
		outputs[CH1_SIGNAL_OUTPUT].setChannels(n);
		
		for (int c = 0; c < n; c++)
			outputs[CH1_SIGNAL_OUTPUT].setVoltage(polarizer1.process(inputs[CH1_SIGNAL_INPUT].getVoltage(c), manual, cv, cvAmount), c);

		lights[CH1_POS_LIGHT].setSmoothBrightness(polarizer1.positiveLevel, args.sampleTime);
		lights[CH1_NEG_LIGHT].setSmoothBrightness(polarizer1.negativeLevel, args.sampleTime);
		
		// channel 2
		manual = params[CH2_MANUAL_PARAM].getValue();
		cv = inputs[CH2_CV_INPUT].getVoltage();
		cvAmount = params[CH2_CVAMOUNT_PARAM].getValue();

		n = inputs[CH2_SIGNAL_INPUT].getChannels();
		outputs[CH2_SIGNAL_OUTPUT].setChannels(n);
		
		for (int c = 0; c < n; c++)
			outputs[CH2_SIGNAL_OUTPUT].setVoltage(polarizer2.process(inputs[CH2_SIGNAL_INPUT].getVoltage(c), manual, cv, cvAmount), c);

		lights[CH2_POS_LIGHT].setSmoothBrightness(polarizer2.positiveLevel, args.sampleTime);
		lights[CH2_NEG_LIGHT].setSmoothBrightness(polarizer2.negativeLevel, args.sampleTime);
	}
};

struct VCPolarizerWidget : ModuleWidget {
	VCPolarizerWidget(VCPolarizer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCPolarizer.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, VCPolarizer::CH1_CVAMOUNT_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, VCPolarizer::CH1_MANUAL_PARAM));
		addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, VCPolarizer::CH2_CVAMOUNT_PARAM));
		addParam(createParamCentered<CountModulaKnobBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, VCPolarizer::CH2_MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCPolarizer::CH1_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCPolarizer::CH2_CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCPolarizer::CH1_SIGNAL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCPolarizer::CH2_SIGNAL_OUTPUT));

		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_NEG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 10, STD_ROWS6[STD_ROW1]), module, VCPolarizer::CH1_POS_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 10, STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_NEG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 10, STD_ROWS6[STD_ROW4]), module, VCPolarizer::CH2_POS_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		VCPolarizer *module = dynamic_cast<VCPolarizer*>(this->module);
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

Model *modelVCPolarizer = createModel<VCPolarizer, VCPolarizerWidget>("VCPolarizer");
