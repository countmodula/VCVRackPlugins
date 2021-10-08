//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Clocked Random Gate CV Expander
//  Adds CV functionality to the Clocked Random Gates module
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/ClockedRandomGateExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ClockedRandomGateExpanderLog
#define PANEL_FILE "ClockedRandomGateExpanderLog.svg"

struct ClockedRandomGateExpanderLog : Module {

	enum ParamIds {
		ENUMS(STEP_LOGIC_PARAMS, CRG_EXP_NUM_CHANNELS),
		SOURCE_PARAM,
		CHANNEL_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		OR_OUTPUT,
		AND_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, CRG_EXP_NUM_CHANNELS),
		AND_LIGHT,
		OR_LIGHT,
		ENUMS(STEP_LOGIC_PARAM_LIGHTS, CRG_EXP_NUM_CHANNELS),
		NUM_LIGHTS
	};
	
	enum TriggerSources {
		TRIGGER_OFF,
		TRIGGER_FROM_GATE,
		TRIGGER_FROM_TRIGGER,
		TRIGGER_FROM_GATED_CLOCK,
		TRIGGER_FROM_CLOCK
	};
	
	// Expander details
	ClockedRandomGateExpanderMessage leftMessages[2][1];	// messages from left module (master)
	ClockedRandomGateExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	ClockedRandomGateExpanderMessage *messagesFromMaster;
	
	bool leftModuleAvailable = false; 

	int channelID = 1; 

	bool singleMode = false;
	bool isPolyphonic = false;

	int numPolyChannels = 1;
	
	bool outcomes[CRG_EXP_NUM_CHANNELS] = {};
	bool clocks[CRG_EXP_NUM_CHANNELS] = {};
	bool triggers[CRG_EXP_NUM_CHANNELS] = {};

	bool prevOutcomes[CRG_EXP_NUM_CHANNELS] = {};
	bool prevClocks[CRG_EXP_NUM_CHANNELS] = {};
	bool prevTriggers[CRG_EXP_NUM_CHANNELS] = {};

	bool outcomesToUse[CRG_EXP_NUM_CHANNELS] = {};
	
	int triggerSource = TRIGGER_FROM_GATE;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ClockedRandomGateExpanderLog() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// step params
		std::string s;
		for (int c = 0; c < CRG_EXP_NUM_CHANNELS; c++) {
			s = "Channel " + std::to_string(c + 1) + " logic";
			configSwitch(STEP_LOGIC_PARAMS + c, 0.0f, 1.0f, 0.0f, s, {"Ignore", "Compare"});
		}
		
		// trigger source switch
		configSwitch(SOURCE_PARAM, 0.0f, 4.0f, 0.0f, "Sync", {"Off", "Gate", "Trigger", "Gated clock", "Clock"});
		
		// trigger channel switch
		configParam(CHANNEL_PARAM, 1.0f, 8.0f, 1.0f, "Source channel");
		
		configOutput(OR_OUTPUT, "OR");
		configOutput(AND_OUTPUT,"AND");
		
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

		bool triggered = false;

		// where are we synchronising from? Irrelevant for single mode so ignore
		channelID = (int)(clamp(params[CHANNEL_PARAM].getValue(), 1.0f, 8.0f));
		triggerSource = (int)(clamp(params[SOURCE_PARAM].getValue(), 0.0f, 4.0f));
		
		// grab the detail from the left hand module if we have one
		leftModuleAvailable = false;
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
					
				leftModuleAvailable = true;
				messagesFromMaster = (ClockedRandomGateExpanderMessage *)(leftExpander.consumerMessage);

				singleMode = messagesFromMaster->singleMode;
				isPolyphonic = messagesFromMaster->isPolyphonic;
				numPolyChannels = messagesFromMaster->numPolyChannels;
				
				for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
					outcomes[i] = messagesFromMaster->gateStates[i];
					clocks[i] = messagesFromMaster->clockStates[i];
					triggers[i] = messagesFromMaster->triggerStates[i];
				}
			
				if (singleMode)
					triggered = true;
				else {
					// trigger on the positive edge of whatever source we've selected
					int c = channelID - 1;
					switch (triggerSource) {
						case TRIGGER_FROM_CLOCK:
							triggered = (clocks[c] && !prevClocks[c]);
							break;
						case TRIGGER_FROM_GATE:
							triggered = (outcomes[c] && !prevOutcomes[c]);
							break;
						case TRIGGER_FROM_TRIGGER:
							triggered = (triggers[c] && !prevTriggers[c]);
							break;
						case TRIGGER_FROM_GATED_CLOCK:
							triggered = (outcomes[c] && clocks[c] && !prevClocks[c]);
							break;
						case TRIGGER_OFF:
							triggered = true;
							break;
					}
				}
			}
		}
		
		if (!leftModuleAvailable) {
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
				outcomes[i] = false;
				clocks[i] = false;
				triggers[i] = false;
				outcomesToUse[i] = false;
			}
		}	

		bool outAND = false;
		bool outOR = false;	

		// step through each bit and set a value based on the bits that match
		short bitMask = 0x01;
		short setBits = 0;
		short outcomeBits = 0;
		for (int c = 0; c < CRG_EXP_NUM_CHANNELS; c++) {
	
			if (triggered)
				outcomesToUse[c] = outcomes[c];

			// set step lights here
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(outcomesToUse[c]));

			// now convert the buttons to logic
			if (params[STEP_LOGIC_PARAMS + c].getValue() > 0.5f)
				setBits = setBits | bitMask;

			// convert the outcomes to logic
			if (outcomesToUse[c])
				outcomeBits = outcomeBits | bitMask;
			
			// prepare for next bit
			bitMask = bitMask << 1;
			
			// save these for next time through
			prevOutcomes[c] = outcomes[c];
			prevClocks[c] = clocks[c];	
			prevTriggers[c] = triggers[c];
		}
		
		// now process the results
		outAND = (setBits > 0 && ((outcomeBits & setBits) == setBits));
		outOR = ((outcomeBits & setBits) != 0);
		
		// set the outputs accordingly
		outputs[OR_OUTPUT].setVoltage(boolToGate(outOR));
		outputs[AND_OUTPUT].setVoltage(boolToGate(outAND));

		// and the lights too
		lights[OR_LIGHT].setBrightness(boolToLight(outOR));
		lights[AND_LIGHT].setBrightness(boolToLight(outAND));	
		
		// finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				ClockedRandomGateExpanderMessage *messageToExpander = (ClockedRandomGateExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// just pass the master module details through
				if (messagesFromMaster) {
					
					messageToExpander->singleMode = singleMode;
					messageToExpander->isPolyphonic = isPolyphonic;
					
					messageToExpander->numPolyChannels = numPolyChannels;
					
					for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i++) {
						messageToExpander->gateStates[i] = outcomes[i];
						messageToExpander->clockStates[i] =  clocks[i];
						messageToExpander->triggerStates[i] = triggers[i];
					}
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct ClockedRandomGateExpanderLogWidget : ModuleWidget {

	std::string panelName;
	
	ClockedRandomGateExpanderLogWidget(ClockedRandomGateExpanderLog *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and knobs
		for (int s = 0; s < CRG_EXP_NUM_CHANNELS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, ClockedRandomGateExpanderLog::STEP_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + s]), module, ClockedRandomGateExpanderLog::STEP_LOGIC_PARAMS + s, ClockedRandomGateExpanderLog::STEP_LOGIC_PARAM_LIGHTS + s));
		}

		// logic lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 20), module, ClockedRandomGateExpanderLog::AND_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 20), module, ClockedRandomGateExpanderLog::OR_LIGHT));

		// source controls
		addParam(createParamCentered<RotarySwitch<OperatingAngle145<GreenKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1],STD_HALF_ROWS8(STD_ROW1)), module, ClockedRandomGateExpanderLog::SOURCE_PARAM));
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, ClockedRandomGateExpanderLog::CHANNEL_PARAM));

		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, ClockedRandomGateExpanderLog::AND_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, ClockedRandomGateExpanderLog::OR_OUTPUT));
	}
	
	struct InitMenuItem : MenuItem {
		ClockedRandomGateExpanderLogWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize logic";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderLog::STEP_LOGIC_PARAMS + i)->getParamQuantity()->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		ClockedRandomGateExpanderLogWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize logic";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderLog::STEP_LOGIC_PARAMS + i)->getParamQuantity()->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	

	// include the theme menu item struct we'll use when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"							
		
	void appendContextMenu(Menu *menu) override {
		ClockedRandomGateExpanderLog *module = dynamic_cast<ClockedRandomGateExpanderLog*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// CV only init
		InitMenuItem *initLogicMenuItem = createMenuItem<InitMenuItem>("Initialize Logic Values Only");
		initLogicMenuItem->widget = this;
		menu->addChild(initLogicMenuItem);

		// CV only random
		RandMenuItem *randLogicMenuItem = createMenuItem<RandMenuItem>("Randomize Logic Values Only");
		randLogicMenuItem->widget = this;
		menu->addChild(randLogicMenuItem);	
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"			
		}
		
		Widget::step();
	}	
	
};

Model *modelClockedRandomGateExpanderLog = createModel<ClockedRandomGateExpanderLog, ClockedRandomGateExpanderLogWidget>("ClockedRandomGateExpanderLog");
