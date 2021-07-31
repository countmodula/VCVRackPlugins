//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Pulse Extender Module
//	A voltage controlled pulse extender
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/PulseModifier.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME GateModifier
#define PANEL_FILE "GateModifier.svg"

struct GateModifier : Module {
	enum ParamIds {
		CV_PARAM,
		LENGTH_PARAM,
		RANGE_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		TRIGGER_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PULSE_OUTPUT,
		END_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		PULSE_LIGHT,
		NUM_LIGHTS
	};

	enum Ranges {
		LONG,
		MEDIUM,
		SHORT
	};
	
	enum Modes {
		RETRIGGER,
		ONESHOT
	};

	GateProcessor gate;
	GateProcessor reset;
	PulseModifier pulse;
	dsp::PulseGenerator pgEnd;
	
	bool isReset = false;
	bool currentState = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	GateModifier() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "CV Amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(LENGTH_PARAM, 0.0f, 10.0f, 0.0f, "Length");
		configParam(RANGE_PARAM, 0.0f, 2.0f, 1.0f, "Range");
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");

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
	
	void onReset() override {
		gate.reset();
		reset.reset();
		pulse.reset();
		isReset = false;
		currentState = false;
	}

	void process(const ProcessArgs &args) override {

		gate.set(inputs[TRIGGER_INPUT].getVoltage());
		reset.set(inputs[RESET_INPUT].getVoltage());
		
		// determine the length - 10 seconds from the knob plus whatever the CV gives us = max 20 seconds
		float length = fmaxf(params[LENGTH_PARAM].getValue() + (clamp(inputs[CV_INPUT].getVoltage(), -10.0f, 10.0f) * params[CV_PARAM].getValue()), 1e-3f);
		
		// apply the range
		switch ((int)(params[RANGE_PARAM].getValue()))
		{
			case LONG:
				// long - up to 20 seconds
				length *= 2.0f;	
				break;
			case MEDIUM:
				// medium - up to 10 seconds
				break;
			case SHORT:
				// short - up to 1 second
				length /= 10.0f; 
				break;
		}
		
		// now update the length in the gate extender
		pulse.set(length);
		
		// determine the mode
		bool retrigger = (params[MODE_PARAM].getValue() < 0.5);
		
		if (reset.high()) {
			// reset sets the output low and disables the timer
			pulse.reset();
			
			//  force us to wait for next trigger before outputting a gate again
			isReset = true;
		}
		else {
			// 
			if (gate.leadingEdge()) {
				isReset = false;
				
				if (!retrigger) {
					// One shot mode, fire off the output pulse
					pulse.restart();
				}
			}
			
			if (!isReset) {
				if (gate.high() && retrigger) {
					// keep restarting the timer until such time as the trigger goes low
					pulse.restart();
				}
				else {
					// tick the timer over
					pulse.process(args.sampleTime);
				}
			}
		}
		
		// process the end trigger
		if(currentState && ! pulse.getState())
			pgEnd.trigger(1e-3f);
		
		// set the outputs
		outputs[PULSE_OUTPUT].setVoltage(boolToGate(pulse.getState()));
		outputs[END_OUTPUT].setVoltage(boolToGate(pgEnd.remaining > 0.0f));
		pgEnd.process(args.sampleTime);
		// set the status light
		lights[PULSE_LIGHT].setSmoothBrightness(boolToLight(pulse.getState()), args.sampleTime);
		
		// save for next time through
		currentState = pulse.getState();
	}
};

struct GateModifierWidget : ModuleWidget {

	std::string panelName;
	
	GateModifierWidget(GateModifier *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, GateModifier::CV_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, GateModifier::LENGTH_PARAM));
		
		//switches
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, GateModifier::RANGE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, GateModifier::MODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, GateModifier::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, GateModifier::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, GateModifier::TRIGGER_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, GateModifier::PULSE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, GateModifier::END_OUTPUT));
		
		
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, GateModifier::PULSE_LIGHT));	
	}

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		GateModifier *module = dynamic_cast<GateModifier*>(this->module);
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

Model *modelGateModifier = createModel<GateModifier, GateModifierWidget>("GateModifier");
