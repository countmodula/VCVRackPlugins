//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Attenuator Module
//	A basic dual attenuator module with one switchable attenuverter and one 
//  simple attenuator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Polarizer.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Attenuator
#define PANEL_FILE "Attenuator.svg"

struct Attenuator : Module {
	enum ParamIds {
		CH1_ATTENUATION_PARAM,
		CH1_MODE_PARAM,
		CH2_ATTENUATION_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_SIGNAL_INPUT,
		CH2_SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH1_SIGNAL_OUTPUT,
		CH2_SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Polarizer polarizer;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Attenuator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CH1_ATTENUATION_PARAM, 0.0f, 1.0f, 0.5f, "Attenuation/Attenuversion");
		configParam(CH2_ATTENUATION_PARAM, 0.0f, 1.0f, 0.0f, "Attenuation", " %", 0.0f, 100.0f, 0.0f);
		configParam(CH1_MODE_PARAM, 0.0f, 1.0f, 0.0f, "Attenuvert");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		polarizer.reset();
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
	
	void process(const ProcessArgs &args) override {
		// grab attenuation settings up front
		float att1 = params[CH1_ATTENUATION_PARAM].getValue();
		float att2 = params[CH2_ATTENUATION_PARAM].getValue();

		bool bipolar = params[CH1_MODE_PARAM].getValue() > 0.5f;
		
		// channel 1
		if (inputs[CH1_SIGNAL_INPUT].isConnected()) {
			// cable connected grab number of channels and process accordingly
			int n = inputs[CH1_SIGNAL_INPUT].getChannels();
			
			outputs[CH1_SIGNAL_OUTPUT].setChannels(n);
			for (int c = 0; c < n; c++) {
				if (bipolar)
					outputs[CH1_SIGNAL_OUTPUT].setVoltage(polarizer.process(inputs[CH1_SIGNAL_INPUT].getVoltage(c), -1.0f + (att1 * 2), 0.0f, 0.0f), c);
				else
					outputs[CH1_SIGNAL_OUTPUT].setVoltage(inputs[CH1_SIGNAL_INPUT].getVoltage(c) * att1, c);
			}
		}
		else {
			// nothing connected, we're acting as a CV source
			if (bipolar)
				outputs[CH1_SIGNAL_OUTPUT].setVoltage(polarizer.process(10.0f, -1.0f + (att1 * 2), 0.0f, 0.0f));
			else
				outputs[CH1_SIGNAL_OUTPUT].setVoltage(10.0f * att1);
		}
		
		// channel 2
		if (inputs[CH2_SIGNAL_INPUT].isConnected()) {
			// cable connected grab number of channels and process accordingly
			int n = inputs[CH2_SIGNAL_INPUT].getChannels();
			
			outputs[CH2_SIGNAL_OUTPUT].setChannels(n);
			for (int c = 0; c < n; c++)
				outputs[CH2_SIGNAL_OUTPUT].setVoltage(inputs[CH2_SIGNAL_INPUT].getVoltage(c) * att2, c);
		}
		else {
			// nothing connected, we're acting as a CV source
			outputs[CH2_SIGNAL_OUTPUT].setVoltage(10.0f * att2);
		}
	
	}
};

struct AttenuatorWidget : ModuleWidget {
	AttenuatorWidget(Attenuator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Attenuator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, Attenuator::CH1_SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, Attenuator::CH2_SIGNAL_INPUT));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, Attenuator::CH1_ATTENUATION_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, Attenuator::CH2_ATTENUATION_PARAM));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, Attenuator::CH1_SIGNAL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, Attenuator::CH2_SIGNAL_OUTPUT));
	
		// switches
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, Attenuator::CH1_MODE_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Attenuator *module = dynamic_cast<Attenuator*>(this->module);
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


Model *modelAttenuator = createModel<Attenuator, AttenuatorWidget>("Attenuator");
