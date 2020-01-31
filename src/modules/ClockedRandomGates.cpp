//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Controlled Clock/Gate Module
//	A voltage controlled clock/gate divider (divide by 1 - approx 20)
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockedRandomGateExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ClockedRandomGates
#define PANEL_FILE "ClockedRandomGates.svg"

struct ClockedRandomGates : Module {
	enum ParamIds {
		ENUMS(PROB_CV_PARAM, CRG_EXP_NUM_CHANNELS),
		ENUMS(PROB_PARAM, CRG_EXP_NUM_CHANNELS),
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		ENUMS(PROB_CV_INPUT, CRG_EXP_NUM_CHANNELS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(GATE_OUTPUT, CRG_EXP_NUM_CHANNELS),
		ENUMS(TRIG_OUTPUT, CRG_EXP_NUM_CHANNELS),
		ENUMS(CLOCK_OUTPUT, CRG_EXP_NUM_CHANNELS),
		POLY_GATE_OUTPUT,
		POLY_TRIG_OUTPUT,
		POLY_CLOCK_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATE_LIGHT, CRG_EXP_NUM_CHANNELS),
		ENUMS(TRIG_LIGHT, CRG_EXP_NUM_CHANNELS),
		ENUMS(CLOCK_LIGHT, CRG_EXP_NUM_CHANNELS),
		NUM_LIGHTS
	};

	GateProcessor gateClock[CRG_EXP_NUM_CHANNELS];
	GateProcessor gateReset;
	
	float probabilities[CRG_EXP_NUM_CHANNELS] = {};
	bool outcomes[CRG_EXP_NUM_CHANNELS] = {};
	bool triggers[CRG_EXP_NUM_CHANNELS] = {};
	
	dsp::PulseGenerator pgTrig[CRG_EXP_NUM_CHANNELS];

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	
	ClockedRandomGates() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Mode");
		
		for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
			configParam(PROB_CV_PARAM + i, -1.0f, 1.0f, 0.0f, "Probability CV Amount", " %", 0.0f, 100.0f, 0.0f);
			configParam(PROB_PARAM + i, 0.0f, 1.0f, 0.5f, "Probability", " %", 0.0f, 100.0f, 0.0f);
		}
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++)
			gateClock[i].reset();
		
		gateReset.reset();
		
		resetOutputs();
	}
	
	void resetOutputs() {
		for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
			outcomes[i] = false;			
			pgTrig[i].reset();
		}
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
		
		// determine the mode and polyphony
		bool singleMode = params[MODE_PARAM].getValue() > 0.5f;
		bool isPolyphonic = inputs[CLOCK_INPUT].isPolyphonic();
		
		// process clock input
		int numPolyChannels = CRG_EXP_NUM_CHANNELS;
		if (singleMode) {
			// single mode can only ever work on 1 clock so always use channel 1
			gateClock[0].set(inputs[CLOCK_INPUT].getNormalVoltage(0.0f));
			
			// if we do have a poly clock input - we'll limit the single mode outputs to the given number of channels
			if (isPolyphonic)
				numPolyChannels = inputs[CLOCK_INPUT].getChannels();
		}
		else if (isPolyphonic) {
			// make sure we handle poly signals the way we want to - we'll wrap around if number of channels < 8 to ensure we always have 8 on the individual outputs
			numPolyChannels = inputs[CLOCK_INPUT].getChannels();
			int j = 0;
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS ; i++) {
				gateClock[i].set(inputs[CLOCK_INPUT].getNormalPolyVoltage(0.0f, j++));
				
				if (j >= numPolyChannels)
					j = 0;
			}
		}
		else {
			// non-poly use single clock always and keep the channels as 8
			gateClock[0].set(inputs[CLOCK_INPUT].getNormalVoltage(0.0f));
		}
		
		// 8 is our limit
		if (numPolyChannels > CRG_EXP_NUM_CHANNELS)
			numPolyChannels = CRG_EXP_NUM_CHANNELS;
		
		// reset input
		gateReset.set(inputs[RESET_INPUT].getNormalVoltage(0.0f));

		// handle the reset - takes precedence over everything else
		if (gateReset.leadingEdge())
			resetOutputs();
		else {
			
			if (singleMode) {
				// single mode - determine outcome for random output accepts single clock input only. If a poly clock is presented, only channel 1 is used 

				if (gateClock[0].leadingEdge()) {
					
					float probSum = 0.0f;
					for (int i = 0; i < numPolyChannels; i++) {
						probabilities[i] = clamp(params[PROB_PARAM + i].getValue() + (inputs[PROB_CV_INPUT + i].getVoltage() * params[PROB_CV_PARAM + i].getValue())/ 10.f, 0.f, 1.f);
						probSum = probSum + probabilities[i];
					}
					
					float r = random::uniform() * probSum;

					float tLow = 0.0f, tHigh = 0.0f;
					for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++) {
						// make sure we don't use any outs beyond the number of input channels
						if (i < numPolyChannels) {
							// upper threshold for this output
							tHigh += probabilities[i];

							bool prevOutcome = outcomes[i];
							outcomes[i] = (probabilities[i] > 0.0f && r > tLow && r <= tHigh);
						
							// fire the trigger here if we've flipped from 0 to 1 on the outcome
							if (outcomes[i] &&  !prevOutcome)
								pgTrig[i].trigger(1e-3f);
							
							// upper threshold for this output becomes lower threshold for next output
							tLow = tHigh;
						}
						else
							outcomes[i] = false;
					}
				}
			}
			else {
				// multi mode - determine the outcome for each output individually
				for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
					
					if (i < numPolyChannels) {
						int j = (isPolyphonic ? i : 0);
						
						if (gateClock[j].leadingEdge()) {
							float r = random::uniform();
							float threshold = clamp(params[PROB_PARAM + i].getValue() + (inputs[PROB_CV_INPUT + i].getVoltage() * params[PROB_CV_PARAM + i].getValue())/ 10.f, 0.f, 1.f);
							
							bool prevOutcome = outcomes[i];
							outcomes[i] = (r < threshold);

							// fire the trigger here if we've flipped from 0 to 1 on the outcome
							if (outcomes[i] &&  !prevOutcome)
								pgTrig[i].trigger(1e-3f);
						}
					}
					else
						outcomes[i] = false;
				}
			}
		}
		
		// now process the outputs and lights
		
		outputs[POLY_GATE_OUTPUT].setChannels(numPolyChannels);
		outputs[POLY_TRIG_OUTPUT].setChannels(numPolyChannels);
		outputs[POLY_CLOCK_OUTPUT].setChannels(numPolyChannels);

		float gate, trig, clock;
		for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
			int j = (singleMode || !isPolyphonic ? 0 : i);
			
			triggers[i] = pgTrig[i].remaining > 0.0f;
			pgTrig[i].process(args.sampleTime);
			
			gate = boolToGate(outcomes[i]);
			trig = boolToGate(triggers[i]);
			clock = boolToGate(outcomes[i] && gateClock[j].high());
			
			lights[GATE_LIGHT + i].setBrightness(boolToLight(outcomes[i]));
			outputs[GATE_OUTPUT +i].setVoltage(gate);

			lights[TRIG_LIGHT + i].setSmoothBrightness(boolToLight(triggers[i]), args.sampleTime);
			outputs[TRIG_OUTPUT +i].setVoltage(trig);
			
			lights[CLOCK_LIGHT + i].setBrightness(boolToLight(outcomes[i] && gateClock[j].high()));
			outputs[CLOCK_OUTPUT +i].setVoltage(clock);
			
			// set the poly outputs
			if (i < numPolyChannels) {
				outputs[POLY_GATE_OUTPUT].setVoltage(gate, i);
				outputs[POLY_TRIG_OUTPUT].setVoltage(trig, i);
				outputs[POLY_CLOCK_OUTPUT].setVoltage(clock, i);
			}
		}
		
		// now set up the expander details
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				ClockedRandomGateExpanderMessage *messageToExpander = (ClockedRandomGateExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// common details
				messageToExpander->singleMode = singleMode;
				messageToExpander->isPolyphonic = isPolyphonic;
				messageToExpander->numPolyChannels = numPolyChannels;
	
				// add the channel specific details
				for (int i = 0; i <  CRG_EXP_NUM_CHANNELS; i++) {
					int j = (singleMode || !isPolyphonic ? 0 : i);
					
					messageToExpander->gateStates[i] = outcomes[i];
					messageToExpander->clockStates[i] =	gateClock[j].high();
					messageToExpander->triggerStates[i] = triggers[i];
				}
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}		
	}
};

struct ClockedRandomGatesWidget : ModuleWidget {
	
	ClockedRandomGatesWidget(ClockedRandomGates *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ClockedRandomGates.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, ClockedRandomGates::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, ClockedRandomGates::RESET_INPUT));
		
		// mode switch
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, ClockedRandomGates::MODE_PARAM));
			
		for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
			// cv inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::PROB_CV_INPUT + i));

			// probability knobs
			addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::PROB_CV_PARAM + i));
			addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::PROB_PARAM + i));
			
			// gate/trig outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9] + 10, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::GATE_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL11] + 10, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::TRIG_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL13] + 10, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::CLOCK_OUTPUT + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL9] - 15, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::GATE_LIGHT + i));
			addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL11] - 15, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::TRIG_LIGHT + i));
			addChild(createLightCentered<MediumLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL13] - 15, STD_ROWS8[STD_ROW1 + i]), module, ClockedRandomGates::CLOCK_LIGHT + i));
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, ClockedRandomGates::POLY_GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, ClockedRandomGates::POLY_TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, ClockedRandomGates::POLY_CLOCK_OUTPUT));
	}
	
	struct InitMenuItem : MenuItem {
		ClockedRandomGatesWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize probabilities";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGates::PROB_PARAM + i)->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		ClockedRandomGatesWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize probabilities";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGates::PROB_PARAM + i)->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ClockedRandomGates *module = dynamic_cast<ClockedRandomGates*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// CV only init
		InitMenuItem *initProbMenuItem = createMenuItem<InitMenuItem>("Initialize Probabilities Only");
		initProbMenuItem->widget = this;
		menu->addChild(initProbMenuItem);

		// CV only random
		RandMenuItem *randProbMenuItem = createMenuItem<RandMenuItem>("Randomize Probabilities Only");
		randProbMenuItem->widget = this;
		menu->addChild(randProbMenuItem);	
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelClockedRandomGates = createModel<ClockedRandomGates, ClockedRandomGatesWidget>("ClockedRandomGates");
