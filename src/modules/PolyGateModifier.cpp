//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Pulse Extender Module
//	A voltage controlled pulse extender
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/PulseModifier.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyGateModifier
#define PANEL_FILE "PolyGateModifier.svg"

struct PolyGateModifier : Module {
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
		ENUMS(INPUT_LIGHTS, 16),
		ENUMS(PULSE_LIGHTS, 16),
		ENUMS(END_LIGHTS, 16),
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

	GateProcessor gate[16];
	GateProcessor reset[16];
	PulseModifier pulse[16];
	dsp::PulseGenerator pgEnd[16];
	
	bool isReset[16] = {};
	bool currentState[16] = {};
	
	float length = 0.0f;
	float lengthCV = 0.0f;
	float range = 1.0f;	
	bool retrigger = true;
	
	int processCount = 8;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	PolyGateModifier() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "CV Amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(LENGTH_PARAM, 0.0f, 10.0f, 0.0f, "Length");
		configSwitch(RANGE_PARAM, 0.0f, 2.0f, 1.0f, "Range", {"Slow", "Medium", "Fast"});
		configSwitch(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode", {"Retrigger", "One-shot"});

		configInput(CV_INPUT, "Length CV");
		configInput(TRIGGER_INPUT, "Gate/trigger");
		configInput(RESET_INPUT, "Reset");

		configOutput(PULSE_OUTPUT, "Modified gate");
		configOutput(END_OUTPUT, "Gate end");

		configBypass(TRIGGER_INPUT, PULSE_OUTPUT);
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"

		range = 1.0f;
		retrigger = true;
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
	
	void onReset() override {
		processCount = 8;
		for (int i = 0; i < 16; i++) {
			gate[i].reset();
			reset[i].reset();
			pulse[i].reset();
			isReset[i] = false;
			currentState[i] = false;
		}
	}

	void process(const ProcessArgs &args) override {

		int numChans =  inputs[TRIGGER_INPUT].getChannels();
		bool polyCV = inputs[CV_INPUT].getChannels() > 1;
		bool polyReset = inputs[RESET_INPUT].getChannels() > 1;
		
		outputs[PULSE_OUTPUT].setChannels(numChans);
		outputs[END_OUTPUT].setChannels(numChans);

		// no need to process controls at audio rate
		if (++processCount > 8) {
			processCount = 0;

			length = params[LENGTH_PARAM].getValue();
			lengthCV = params[CV_PARAM].getValue();

			// apply the range
			range = 1.0f;
			switch ((int)(params[RANGE_PARAM].getValue()))
			{
				case LONG:
					// long - up to 20 seconds
					range = 2.0f;
					break;
				case MEDIUM:
					// medium - up to 10 seconds
					break;
				case SHORT:
					// short - up to 1 second
					range = 0.1f; 
					break;
			}
			
			// determine the mode
			retrigger = (params[MODE_PARAM].getValue() < 0.5);
		}

		for (int i = 0; i < 16; i++) {
			if (i < numChans) {

				reset[i].set(polyReset ? inputs[RESET_INPUT].getPolyVoltage(i) : inputs[RESET_INPUT].getVoltage());
				
				if (processCount == 0) {
					gate[i].set(inputs[TRIGGER_INPUT].getVoltage(i));

					// determine the pulse length - 10 seconds from the knob plus whatever the CV gives us = max 20 seconds
					float l = range * fmaxf(length + clamp(polyCV ? inputs[CV_INPUT].getPolyVoltage(i) : inputs[CV_INPUT].getVoltage(),  -10.0f, 10.0f) * lengthCV, 1e-3f);
					pulse[i].set(l);
				}

				lights[INPUT_LIGHTS + i].setSmoothBrightness(boolToLight(gate[i].high()), args.sampleTime);

				if (reset[i].high()) {
					// reset sets the output low and disables the timer
					pulse[i].reset();
					
					// force us to wait for next trigger before outputting a gate again
					isReset[i] = true;
				}
				else {
					if (gate[i].leadingEdge()) {
						isReset[i] = false;
						
						if (!retrigger) {
							// One shot mode, fire off the output pulse
							pulse[i].restart();
						}
					}
					
					if (!isReset[i]) {
						if (gate[i].high() && retrigger) {
							// keep restarting the timer until such time as the trigger goes low
							pulse[i].restart();
						}
						else {
							// tick the timer over
							pulse[i].process(args.sampleTime);
						}
					}
				}

				// process the end trigger
				if(currentState[i] && ! pulse[i].getState()) {
					pgEnd[i].trigger(1e-3f);
					outputs[END_OUTPUT].setVoltage(10.0f, i);
					//lights[END_LIGHTS + i].setBrightness(1.0f);
				}
				else if (pgEnd[i].process(args.sampleTime)) {
					outputs[END_OUTPUT].setVoltage(10.0f, i);
					//lights[END_LIGHTS + i].setBrightness(1.0f);
				}
				else {
					outputs[END_OUTPUT].setVoltage(0.0f, i);
					//lights[END_LIGHTS + i].setSmoothBrightness(0.0f, args.sampleTime);
				}

				// save for next time through
				currentState[i] = pulse[i].getState();

				// set the pulse output and lights
				if (currentState[i]) {
					outputs[PULSE_OUTPUT].setVoltage(10.0f, i);
					//lights[PULSE_LIGHTS + i].setBrightness(1.0f);
				}
				else {
					outputs[PULSE_OUTPUT].setVoltage(0.0f, i);
					//lights[PULSE_LIGHTS + i].setSmoothBrightness(0.0f, args.sampleTime);
				}
				
				if (processCount == 0) {
					float elapsed = args.sampleTime * 2.0;
					lights[PULSE_LIGHTS + i].setSmoothBrightness(boolToLight(currentState[i]), elapsed);
					lights[END_LIGHTS + i].setSmoothBrightness(boolToLight(pgEnd[i].remaining > 0.0f), elapsed);
				}
			}
			else {
				gate[i].set(0.0f);
				reset[i].set(0.0f);
				pgEnd[i].reset();
				currentState[i] = false;
				isReset[i] = false;
				
				if (processCount == 0) {
					lights[INPUT_LIGHTS + i].setBrightness(0.0f);
					lights[PULSE_LIGHTS + i].setBrightness(0.0f);
					lights[END_LIGHTS + i].setBrightness(0.0f);
				}
			}
		}
	}
};

struct PolyGateModifierWidget : ModuleWidget {

	std::string panelName;
	
	PolyGateModifierWidget(PolyGateModifier *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, PolyGateModifier::LENGTH_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, PolyGateModifier::CV_PARAM));
		
		//switches
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, PolyGateModifier::MODE_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, PolyGateModifier::RANGE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, PolyGateModifier::TRIGGER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, PolyGateModifier::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, PolyGateModifier::CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, PolyGateModifier::PULSE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, PolyGateModifier::END_OUTPUT));
		
		addLEDMatrix(PolyGateModifier::INPUT_LIGHTS, STD_COL3, STD_ROW1, 0);
		addLEDMatrix(PolyGateModifier::PULSE_LIGHTS, STD_COL3, STD_ROW5, 0);
		addLEDMatrix(PolyGateModifier::END_LIGHTS, STD_COL3, STD_ROW6, 0);
	}

	void addLEDMatrix(int lightID, int colPos, int rowPos, int rowOffset) { 
		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS6[rowPos] - 15 + rowOffset;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[colPos] - 15 + (10 * x++), startPos + (10 * y)), module, lightID + s));
		}	
	}


	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyGateModifier *module = dynamic_cast<PolyGateModifier*>(this->module);
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

Model *modelPolyGateModifier = createModel<PolyGateModifier, PolyGateModifierWidget>("PolyGateModifier");
