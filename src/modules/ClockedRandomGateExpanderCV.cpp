//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Clocked Random Gate CV Expander
//	Adds CV functionality to the Clocked Random Gates module
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/ClockedRandomGateExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ClockedRandomGateExpanderCV
#define PANEL_FILE "ClockedRandomGateExpanderCV.svg"

struct ClockedRandomGateExpanderCV : Module {

	enum ParamIds {
		ENUMS(STEP_CV_PARAMS, CRG_EXP_NUM_CHANNELS),
		RANGE_SW_PARAM,
		SOURCE_PARAM,
		CHANNEL_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		CV_OUTPUT,
		CVI_OUTPUT,
		PULSE_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, CRG_EXP_NUM_CHANNELS),
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
	
	ClockedRandomGateExpanderCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// step params
		for (int s = 0; s < CRG_EXP_NUM_CHANNELS; s++) {
			configParam(STEP_CV_PARAMS + s, 0.0f, 8.0f, (float)(s + 1), "Step value");
		}
		
		// trigger source switch
		configSwitch(SOURCE_PARAM, 0.0f, 4.0f, 0.0f, "S&H trigger", {"Off", "Gate", "Trigger", "Gated clock", "Clock"});
		
		// trigger channel switch
		configParam(CHANNEL_PARAM, 1.0f, 8.0f, 1.0f, "Trigger channel");
		
		// range switch
		configSwitch(RANGE_SW_PARAM, 0.0f, 2.0f, 0.0f, "Scale", {"8V", "4V", "2V"});

		configOutput(PULSE_OUTPUT, "Trigger");
		configOutput(CV_OUTPUT, "CV");
		configOutput(CVI_OUTPUT,"Inverted CV");

		outputInfos[PULSE_OUTPUT]->description = "Outputs the selected S&H trigger";
		
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
	
	float getScale(float range) {
		
		switch ((int)(range)) {
			case 2:
				return 0.25f; // 2 volts
			case 1:
				return 0.5f; // 4 volts
			case 0:
			default:
				return 1.0f; // 8 volts
		}
	}

	void process(const ProcessArgs &args) override {

		bool triggered = false;

		// where are we deriving our sample and hold trigger from? Irrelevant for single mode so ignore
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

		// determine which scale to use
		float scale = getScale(params[RANGE_SW_PARAM].getValue());

		// adjust if we're in multi mode so the switch determines the max value of all steps on
		if (!singleMode)
			scale = scale / 8.0f;

		// Determine the output pulse from the S&H trigger source
		bool pulseOut = false;
		if (triggerSource != TRIGGER_OFF) {
			if (singleMode) {
				// if we're in single mode, we have to use the driving clock from the master which is always channel 1
				pulseOut = clocks[0];
			}
			else {
				int c = channelID - 1;
				switch (triggerSource) {
					case TRIGGER_FROM_GATE:
						pulseOut = outcomes[c];
						break;
					case TRIGGER_FROM_TRIGGER:
						pulseOut = triggers[c];
						break;					
					case TRIGGER_FROM_GATED_CLOCK:
						pulseOut = (outcomes[c] && clocks[c]);
						break;
					case TRIGGER_FROM_CLOCK:
						pulseOut = clocks[c];
						break;
				}	
			}
		}		
		
		// now deal with the CV and lights
		float cv = 0.0f;
		for (int c = 0; c < CRG_EXP_NUM_CHANNELS; c++) {
	
			if (triggered)
				outcomesToUse[c] = outcomes[c];
			
			// grab the cv value
			if (outcomesToUse[c])
			{
				cv += params[STEP_CV_PARAMS + c].getValue() * scale;

				// set step lights here
				lights[STEP_LIGHTS + c].setBrightness(1.0f);
			}
			else {
					lights[STEP_LIGHTS + c].setBrightness(0.0f);
			}
			
			// save these for next time through
			prevOutcomes[c] = outcomes[c];
			prevClocks[c] = clocks[c];	
			prevTriggers[c] = triggers[c];
		}
		
		// set the outputs accordingly
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
		
		outputs[PULSE_OUTPUT].setVoltage(boolToGate(pulseOut));
		
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

struct ClockedRandomGateExpanderCVWidget : ModuleWidget {

	std::string panelName;
	
	ClockedRandomGateExpanderCVWidget(ClockedRandomGateExpanderCV *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and knobs
		for (int s = 0; s < CRG_EXP_NUM_CHANNELS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, ClockedRandomGateExpanderCV::STEP_LIGHTS + s));
			addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + s]), module, ClockedRandomGateExpanderCV::STEP_CV_PARAMS + s));
		}

		// source controls
		addParam(createParamCentered<RotarySwitch<OperatingAngle145<GreenKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1],STD_HALF_ROWS8(STD_ROW1)), module, ClockedRandomGateExpanderCV::SOURCE_PARAM));
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, ClockedRandomGateExpanderCV::CHANNEL_PARAM));

		// range control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, ClockedRandomGateExpanderCV::RANGE_SW_PARAM));
		
		// S&H Pulse output
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW4)), module, ClockedRandomGateExpanderCV::PULSE_OUTPUT));
		
		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, ClockedRandomGateExpanderCV::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, ClockedRandomGateExpanderCV::CVI_OUTPUT));
	}
	
	struct ZeroMenuItem : MenuItem {
		ClockedRandomGateExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "zero cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderCV::STEP_CV_PARAMS + i)->getParamQuantity()->setValue(0.0f);

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};		
	

	struct BinaryMenuItem : MenuItem {
		ClockedRandomGateExpanderCVWidget *widget;
		float binaryWeights[8] = {0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f};

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "binary weight cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderCV::STEP_CV_PARAMS + i)->getParamQuantity()->setValue(binaryWeights[i]);

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};		
		
	
	struct InitMenuItem : MenuItem {
		ClockedRandomGateExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderCV::STEP_CV_PARAMS + i)->getParamQuantity()->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		ClockedRandomGateExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int i = 0; i < CRG_EXP_NUM_CHANNELS; i ++)
				widget->getParam(ClockedRandomGateExpanderCV::STEP_CV_PARAMS + i)->getParamQuantity()->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	

	// include the theme menu item struct we'll use when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"							
		
	void appendContextMenu(Menu *menu) override {
		ClockedRandomGateExpanderCV *module = dynamic_cast<ClockedRandomGateExpanderCV*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));
		
		// CV only init
		InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("Initialize CV Values Only");
		initCVMenuItem->widget = this;
		menu->addChild(initCVMenuItem);

		// CV only random
		RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("Randomize CV Values Only");
		randCVMenuItem->widget = this;
		menu->addChild(randCVMenuItem);	

		// CV only zero
		ZeroMenuItem *zeroCVMenuItem = createMenuItem<ZeroMenuItem>("Zero CV Values");
		zeroCVMenuItem->widget = this;
		menu->addChild(zeroCVMenuItem);				

		// CV only zero
		BinaryMenuItem *binaryCVMenuItem = createMenuItem<BinaryMenuItem>("Binary Weight CV Values");
		binaryCVMenuItem->widget = this;
		menu->addChild(binaryCVMenuItem);				
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"			
		}
		
		Widget::step();
	}	
	
};

Model *modelClockedRandomGateExpanderCV = createModel<ClockedRandomGateExpanderCV, ClockedRandomGateExpanderCVWidget>("ClockedRandomGateExpanderCV");
