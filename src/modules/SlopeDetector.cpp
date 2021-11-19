//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - SlopeDetector Module
//	A VCV rack implementation of the CGS Slope Detector designed by Ken Stone
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SlewLimiter.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SlopeDetector
#define PANEL_FILE "SlopeDetector.svg"

struct SlopeDetector : Module {
	enum ParamIds {
		SENSE_PARAM,
		RANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		RISING_OUTPUT,
		STEADY_OUTPUT,
		FALLING_OUTPUT,
		MOVING_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RISING_LIGHT,
		STEADY_LIGHT,
		FALLING_LIGHT,
		MOVING_LIGHT,
		NUM_LIGHTS
	};

	LagProcessor lag;
	
	float cv = 0.0f, cvLag = 0.0f;
	float lagAmt = 0.0f, range = 0.0f;
	
	int count = 0;
	bool prevRising = false, prevFalling = false;
	bool rising = false, falling = false, steady = true;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SlopeDetector() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(SENSE_PARAM, 0.0f, 1.0f, 0.0f, "Sense");
		configSwitch(RANGE_PARAM, 0.0f, 1.0f, 0.0f, "Sense range", {"Fast", "Slow"});

		configInput(CV_INPUT, "Signal");

		configOutput(RISING_OUTPUT, "Rising");
		configOutput(STEADY_OUTPUT, "Steady");
		configOutput(FALLING_OUTPUT, "Falling");
		configOutput(MOVING_OUTPUT, "Moving");

		outputInfos[RISING_OUTPUT]->description = "High gate when the input signal is rising";
		outputInfos[STEADY_OUTPUT]->description = "High gate when the input signal is steady";
		outputInfos[FALLING_OUTPUT]->description = "High gate when the input signal is falling";
		outputInfos[MOVING_OUTPUT]->description = "High gate when the input signal is rising or falling";

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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
		// grab CV input value
		cv = inputs[CV_INPUT].getVoltage();

		// apply lag
		range = (params[RANGE_PARAM].getValue() > 0.5f ? 1.0f : 0.5f);
		lagAmt = (0.1f + params[SENSE_PARAM].getValue()) * range;
		cvLag = lag.process(cv, 1.0f, lagAmt, lagAmt, args.sampleTime);
		
		// determine if we're rising or falling based on the lagged value
		// applying a bit of hysteresis to avoid double triggering.
		// double triggering will still happen if the response is set too quick and the input signal is slow
		if(fabs(cv - cvLag) > 0.01f) {
			if (cv > cvLag) {
				rising = true;
			}
			else if (prevRising && cv < (cvLag - 0.01f)) {
				rising = false;
			}
			
			if (cv < cvLag) {
				falling = true;
			}
			else if (prevFalling && cv > (cvLag + 0.01f)) {
				falling = false;
			}
		}
		else {
			// any cv input within 0.01 volts of the lagged value is considered steady
			rising = falling = false;
		}
		
		// steady only if neither rising or falling
		steady = !(rising || falling);
		
		// set the outputs
		outputs[RISING_OUTPUT].setVoltage(boolToGate(rising));
		outputs[STEADY_OUTPUT].setVoltage(boolToGate(steady));
		outputs[FALLING_OUTPUT].setVoltage(boolToGate(falling));
		outputs[MOVING_OUTPUT].setVoltage(boolToGate(!steady));

		// Set lights
		lights[RISING_LIGHT].setSmoothBrightness(boolToLight(rising), args.sampleTime);
		lights[STEADY_LIGHT].setSmoothBrightness(boolToLight(steady), args.sampleTime);
		lights[FALLING_LIGHT].setSmoothBrightness(boolToLight(falling), args.sampleTime);
		lights[MOVING_LIGHT].setSmoothBrightness(boolToLight(!steady), args.sampleTime);
		
		// use these to detect change of direction
		prevRising = rising;
		prevFalling = falling;
	}
};

struct SlopeDetectorWidget : ModuleWidget {

	std::string panelName;
	
	SlopeDetectorWidget(SlopeDetector *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW1]), module, SlopeDetector::CV_INPUT));

		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, SlopeDetector::SENSE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, SlopeDetector::RANGE_PARAM));

		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, SlopeDetector::RISING_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, SlopeDetector::STEADY_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, SlopeDetector::FALLING_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SlopeDetector::MOVING_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, SlopeDetector::RISING_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, SlopeDetector::STEADY_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, SlopeDetector::FALLING_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, SlopeDetector::MOVING_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SlopeDetector *module = dynamic_cast<SlopeDetector*>(this->module);
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

Model *modelSlopeDetector = createModel<SlopeDetector, SlopeDetectorWidget>("SlopeDetector");
