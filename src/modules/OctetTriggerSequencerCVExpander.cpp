//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Octet Trigger Sequencer CV expander
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/OctetTriggerSequencerExpanderMessage.hpp"

#define STRUCT_NAME OctetTriggerSequencerCVExpander
#define WIDGET_NAME OctetTriggerSequencerCVExpanderWidget
#define MODULE_NAME "OctetTriggerSequencerCVExpander"
#define PANEL_FILE "OctetTriggerSequencerCVExpander.svg"
#define MODEL_NAME	modelOctetTriggerSequencerCVExpander

// set the module name for the theme selection functions
#define THEME_MODULE_NAME OctetTriggerSequencerCVExpander

struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(CVA_PARAMS, 8),
		ENUMS(CVB_PARAMS, 8),
		RANGE_SW_PARAM,
		HOLD_PARAM,		
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		CVA_OUTPUT,
		CVAI_OUTPUT,
		CVB_OUTPUT,
		CVBI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEPA_LIGHTS, 16),
		ENUMS(STEPB_LIGHTS, 16),
		NUM_LIGHTS
	};
	
	// avalable pattern modes
	enum ChainedPatternModes {
		CHAINED_MODE_B_OFF,
		CHAINED_MODE_B_FOLLOW,
		CHAINED_MODE_B_INVERSE,
		NUM_PATTERN_MODES
	};
	
	int count;
	int currentChannel = 0;
	int userChannel = 0;
	int prevChannel = 0;
	bool doRedraw = true;
	int processCount = 0;
	
	float cvA = 0.0f;
	float cvB = 0.0f;
	bool hold = false;
	bool gateA =false;
	bool gateB = false;
	int selectedPatternA = 0;
	int selectedPatternB = 0;
	int chainedPatternMode = 0;

	// Expander details
	OctetTriggerSequencerExpanderMessage leftMessages[2][1];		// messages from left module (master)
	OctetTriggerSequencerExpanderMessage rightMessages[2][1]; 	// messages to right module (expander)
	OctetTriggerSequencerExpanderMessage *messagesFromMaster;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"

	// count to bit mappping
	STEP_MAP;

	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// cv/gate params
		char stepText[20];
		for (int s = 0; s < 8; s++) {
			sprintf(stepText, "Step %d value", s + 1);
			configParam(CVA_PARAMS + s, 0.0f, 1.0f, 0.0f, stepText, " V", 0.0f, 8.0f, 0.0f);
			configParam(CVB_PARAMS + s, 0.0f, 1.0f, 0.0f, stepText, " V", 0.0f, 8.0f, 0.0f);
		}
		
		// range switch
		configParam(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Output scale");
		
		// hold mode switch
		configSwitch(HOLD_PARAM, 0.0f, 1.0f, 0.0f, "Sample and hold", {"Off", "On"});
		
		configOutput(CVA_OUTPUT, "CV A");
		configOutput(CVAI_OUTPUT, "Inverted CV A");
		configOutput(CVB_OUTPUT, "CV B");
		configOutput(CVBI_OUTPUT, "Inverted CV B");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "channel", json_integer(userChannel));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
		
		json_t *ch = json_object_get(root, "channel");

		if (ch)
			userChannel = json_integer_value(ch);
	
		doRedraw = true;
	}

	void process(const ProcessArgs &args) override {

		bool clockEdge = false;
		bool chained = false, playingChannelB = false;
		count = 0;

		// grab the detail from the left hand module if we have one
		currentChannel = 0;
		messagesFromMaster = 0; 
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
				messagesFromMaster = (OctetTriggerSequencerExpanderMessage *)(leftExpander.consumerMessage);

				count = messagesFromMaster->counter;
				clockEdge = messagesFromMaster->clockEdge;
				selectedPatternA = messagesFromMaster->selectedPatternA;
				selectedPatternB = messagesFromMaster->selectedPatternB;
				chained = messagesFromMaster->chained;
				playingChannelB = messagesFromMaster->playingChannelB;
				chainedPatternMode = messagesFromMaster->chainedPatternMode;
				processCount = messagesFromMaster->processCount;
				gateA = messagesFromMaster ->gateA;
				gateB = messagesFromMaster ->gateB;
				
				if (userChannel == 0)
					userChannel = messagesFromMaster->channel;
				
				if (messagesFromMaster->hasMaster)
					currentChannel = userChannel;
			}
			else if (++processCount > 8)
				processCount = 0;
		}
		else if (++processCount > 8)
			processCount = 0;

		if (currentChannel != prevChannel) {
			doRedraw = true;
			prevChannel = currentChannel;
		}
		
		// determine which scale to use
		float scale = params[RANGE_SW_PARAM].getValue();

		// no need to check for pattern change, and update display at audio rate
		if (processCount == 0) {
			hold = params[HOLD_PARAM].getValue() > 0.5f;
			
			int led = 0;
			int stepLed = 1;
			bool activeA, activeB;
			bool stepA, stepB;

			// display the pattern and clock values
			for (int b = 1; b < 9; b++) {
				activeA = ((selectedPatternA & stepMap[b]) > 0);
				activeB = ((selectedPatternB & stepMap[b]) > 0);

				stepA = (count == b);
				stepB = (count == b);

				if (chained) {
					stepA &= (!playingChannelB);
					stepB &= playingChannelB;
				}

				lights[STEPA_LIGHTS + led].setBrightness(boolToLight(activeA));
				lights[STEPB_LIGHTS + led].setBrightness(boolToLight(activeB));
				lights[STEPA_LIGHTS + stepLed].setBrightness(boolToLight(stepA));
				lights[STEPB_LIGHTS + stepLed].setBrightness(boolToLight(stepB));

				led += 2;
				stepLed += 2;
			}
		}
		
		if (clockEdge) {
			float nextA = 0.0f;
			float nextB = 0.0f;
			int c = count -1;

			if (count > 0) {
				if (chained) {
					// if playing channel B, then channel A's next value is channel B's control value otherwise it is channel A's control value.
					nextA = playingChannelB ? params[CVB_PARAMS + c].getValue() : params[CVA_PARAMS + c].getValue();

					// if channel B's pattern mode is off, channel B's next value is 0 volts
					// if channel B's pattern mode is follow or inverse, channel B's next value is channel A's next value
					if (chainedPatternMode != CHAINED_MODE_B_OFF)
						nextB = nextA;
				}
				else {
					// next values are as per the A/B channel control values
					nextA = params[CVA_PARAMS + c].getValue();
					nextB = params[CVB_PARAMS + c].getValue();
				}
			}
			
			if (hold) {
				if (gateA)
					cvA = nextA;
					
				if (gateB)
					cvB = nextB;
			}
			else {
				cvA = nextA;
				cvB = nextB;
			}
		}
		
		// now we can set the outputs and lights
		outputs[CVA_OUTPUT].setVoltage(cvA * scale);
		outputs[CVAI_OUTPUT].setVoltage(-cvA * scale);
		outputs[CVB_OUTPUT].setVoltage(cvB * scale);
		outputs[CVBI_OUTPUT].setVoltage(-cvB * scale);

		// // finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				OctetTriggerSequencerExpanderMessage *messageToExpander = (OctetTriggerSequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				if (messagesFromMaster) {
					int ch = 0;
					if (messagesFromMaster->hasMaster) {
						ch = messagesFromMaster->channel;

						if (++ch > 7)
							ch = 1;
					}
						
					messageToExpander->set(count, clockEdge, selectedPatternA, selectedPatternB, ch, messagesFromMaster->hasMaster, playingChannelB, chained, chainedPatternMode, processCount, gateA, gateB);
				}
				else {
					messageToExpander->initialise();
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// step pots and lights
		int led = 0;
		for (int s = 0; s < 8; s++) {
			addChild(createLightCentered<MediumLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL1]-15, STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME::STEPA_LIGHTS + led));
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL2]-15, STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME:: CVA_PARAMS + s));
			addChild(createLightCentered<MediumLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME::STEPB_LIGHTS + led));
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + s]), module, STRUCT_NAME:: CVB_PARAMS + s));
			led += 2;
		}

		// range control
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW3]), module, STRUCT_NAME::RANGE_SW_PARAM));
			
		// hold mode control
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME::HOLD_PARAM));
			
		// output jacks
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW5]), module, STRUCT_NAME::CVA_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW6]), module, STRUCT_NAME::CVAI_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::CVB_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::CVBI_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	// Channel selection menu item
	struct ChannelMenuItem : MenuItem {
		STRUCT_NAME *module;
		int channelToUse = 0;
		
		void onAction(const event::Action &e) override {
			module->userChannel = channelToUse;
		}
	};	
		
	// channel menu item
	struct ChannelMenu : MenuItem {
		STRUCT_NAME *module;		
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
		
			char buffer[20];
			for (int i = 1; i < 8; i++) {
				sprintf(buffer, "Channel %d (%s)", i, CountModulaknobColours[i]);
				ChannelMenuItem *channelMenuItem = createMenuItem<ChannelMenuItem>(buffer, CHECKMARK(module->userChannel == i));
				channelMenuItem->module = module;
				channelMenuItem->channelToUse = i;
				menu->addChild(channelMenuItem);
			}
			
			return menu;	
		}
	};	
		
	// initialize selection menu item
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 3;
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			switch (channel) {
				case 0:
					h->name = "channel A";
					break;
				case 1:
					h->name = "channel B";
					break;
				default:
					h->name = "both channels";
					break;
			}

			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int c = 0; c < 8; c++) {
				if ((channel& 0x01) == 0x01)
					widget->getParam(STRUCT_NAME::CVA_PARAMS + c)->getParamQuantity()->reset();
				
				if ((channel& 0x02) == 0x02)
					widget->getParam(STRUCT_NAME::CVB_PARAMS + c)->getParamQuantity()->reset();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// initialize menu item
	struct InitMenu : MenuItem {
		WIDGET_NAME *widget;		
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			InitMenuItem *initCVAMenuItem = createMenuItem<InitMenuItem>("Channel A");
			initCVAMenuItem->widget = widget;
			initCVAMenuItem->channel = 1;
			menu->addChild(initCVAMenuItem);

			InitMenuItem *initCVBMenuItem = createMenuItem<InitMenuItem>("Channel B");
			initCVBMenuItem->widget = widget;
			initCVBMenuItem->channel = 2;
			menu->addChild(initCVBMenuItem);
			
			InitMenuItem *initCVABothMenuItem = createMenuItem<InitMenuItem>("Both channels");
			initCVABothMenuItem->widget = widget;
			initCVABothMenuItem->channel = 3;
			menu->addChild(initCVABothMenuItem);
			
			return menu;	
		}
	};	
	
	// randomize selection menu item
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;

		int channel = 3;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			switch (channel) {
				case 0:
					h->name = "channel A";
					break;
				case 1:
					h->name = "channel B";
					break;
				default:
					h->name = "both channels";
					break;
			}

			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int c = 0; c < 8; c++) {

				if ((channel& 0x01) == 0x01)
					widget->getParam(STRUCT_NAME::CVA_PARAMS + c)->getParamQuantity()->randomize();
					
				if ((channel& 0x02) == 0x02)
					widget->getParam(STRUCT_NAME::CVB_PARAMS + c)->getParamQuantity()->randomize();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// randomize menu item
	struct RandMenu : MenuItem {
		WIDGET_NAME *widget;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			RandMenuItem *randCVaMenuItem = createMenuItem<RandMenuItem>("Channel A");
			randCVaMenuItem->widget = widget;
			randCVaMenuItem->channel = 1;
			menu->addChild(randCVaMenuItem);

			RandMenuItem *randCVBMenuItem = createMenuItem<RandMenuItem>("Channel B");
			randCVBMenuItem->widget = widget;
			randCVBMenuItem->channel = 2;
			menu->addChild(randCVBMenuItem);

			RandMenuItem *randCVAllMenuItem = createMenuItem<RandMenuItem>("Both channels");
			randCVAllMenuItem->widget = widget;
			randCVAllMenuItem->channel = 3;
			menu->addChild(randCVAllMenuItem);

			return menu;	
		}
	};	
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());

		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// add channel menu item
		ChannelMenu *channelMenuItem = createMenuItem<ChannelMenu>("Channel", RIGHT_ARROW);
		channelMenuItem->module = module;
		menu->addChild(channelMenuItem);
		
		// add initialize menu item
		InitMenu *initMenuItem = createMenuItem<InitMenu>("Initialize", RIGHT_ARROW);
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// add randomize menu item
		RandMenu *randMenuItem = createMenuItem<RandMenu>("Randomize", RIGHT_ARROW);
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);
	}

	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
	
			if (((STRUCT_NAME*)module)->doRedraw) {
				int cid = ((STRUCT_NAME*)module)->currentChannel;
				
				char buffer[50];
				sprintf(buffer, "res/Components/Knob%s.svg", CountModulaknobColours[cid]);
				
				for (int i = 0; i < 8; i++) {
					CountModulaKnob *pA = (CountModulaKnob *)getParam(STRUCT_NAME::CVA_PARAMS + i);
					pA->svgFile = CountModulaknobColours[cid];
					pA->setSvg(Svg::load(asset::plugin(pluginInstance, buffer))); 
					pA->fb->dirty = true;
					
					CountModulaKnob *pB = (CountModulaKnob *)getParam(STRUCT_NAME::CVB_PARAMS + i);
					pB->svgFile = CountModulaknobColours[cid];
					pB->setSvg(Svg::load(asset::plugin(pluginInstance, buffer))); 
					pB->fb->dirty = true;
				}
			}
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
