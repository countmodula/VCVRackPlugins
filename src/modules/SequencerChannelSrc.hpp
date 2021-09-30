//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard Sequencer Channel Engine
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, SEQ_NUM_STEPS),
		ENUMS(CV_PARAMS, SEQ_NUM_STEPS),
		RANGE_SW_PARAM,
		HOLD_PARAM,		
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		TRIG_OUTPUT,
		END_OUTPUT,
		CV_OUTPUT,
		CVI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		GATE_LIGHT,
		TRIG_LIGHT,
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(DIRECTION_LIGHTS, 5),
		NUM_LIGHTS
	};
	
	int count;
	int length;
	int currentChannel = 0;
	int userChannel = 0;
	int prevChannel = 0;
	bool doRedraw = true;
	
	float cv = 0.0f;
	bool prevGate = false;

	// Expander details
	SequencerChannelMessage leftMessages[2][1];	// messages from left module (master)
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerChannelMessage *messagesFromMaster;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	char knobColours[8][50] = {	"Grey", 
								"Red", 
								"Orange",  
								"Yellow", 
								"Blue", 
								"Violet",
								"White",
								"Green"};		
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// cv/gate params
		char stepText[20];
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			sprintf(stepText, "Step %d select", s + 1);
			configParam(STEP_PARAMS + s, 0.0f, 2.0f, 1.0f, stepText);
			
			sprintf(stepText, "Step %d value", s + 1);
			configParam(CV_PARAMS + s, 0.0f, 1.0f, 0.0f, stepText, " V", 0.0f, 8.0f, 0.0f);
		}
		
		// range switch
		configParam(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale");
		
		// hold mode switch
		configParam(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Sample and hold mode");
		
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

		bool running = false;
		bool clock = false;
		count = 0;

		// grab the detail from the left hand module if we have one
		currentChannel = 0;
		messagesFromMaster = 0; 
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
					
				messagesFromMaster = (SequencerChannelMessage *)(leftExpander.consumerMessage);

				count = messagesFromMaster->counter;
				length = messagesFromMaster->length;
				running = messagesFromMaster->runningState;
				clock = messagesFromMaster->clockState;
				
				if (userChannel == 0)
					userChannel = messagesFromMaster->channel;
				
				if (messagesFromMaster->hasMaster)
					currentChannel = userChannel;
			}
		}
		
		if (currentChannel != prevChannel) {
			doRedraw = true;
			prevChannel = currentChannel;
		}
		
		bool gate = false, trig = false;
		
		// determine which scale to use
		float scale = params[RANGE_SW_PARAM].getValue();

		// process the step switches, cv and set the length/active step lights etc
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {

			// set step and length lights here
			bool stepActive = (c + 1 == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			lights[LENGTH_LIGHTS + c].setBrightness(boolToLight(c < length));

			// process the gate and CV for the current step
			if (stepActive) {
				switch ((int)(params[STEP_PARAMS + c].getValue())) {
					case 0:
						gate = true;
						break;
					case 2:
						trig = true;
						break;
				}
			
				// now grab the cv value
				switch ((int)(params[HOLD_PARAM].getValue())) {
					case 0: // on trig
						cv = trig ? params[CV_PARAMS + c].getValue(): cv;
						break;
					
					case 2: // on gate
						cv = (gate && ! prevGate) ? params[CV_PARAMS + c].getValue(): cv;
						break;
					case 1: // off
					default:
						cv = params[CV_PARAMS + c].getValue();
						break;
				}
			}
		}
		
		// now we can set the outputs and lights
		outputs[GATE_OUTPUT].setVoltage(boolToGate(gate));
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig && clock));
		lights[GATE_LIGHT].setBrightness(boolToLight(gate));
		lights[TRIG_LIGHT].setBrightness(boolToLight(trig && clock));
		
		outputs[CV_OUTPUT].setVoltage(cv  * scale);
		outputs[CVI_OUTPUT].setVoltage(-cv  * scale);
		
		prevGate = gate;
		
		// finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerChannelMessage *messageToExpander = (SequencerChannelMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				if (messagesFromMaster) {
					int ch = 0;
					if (messagesFromMaster->hasMaster) {
						ch = messagesFromMaster->channel;

						if (++ch > 7)
							ch = 1;
					}
						
					messageToExpander->set(count, length, clock, running, ch, messagesFromMaster->hasMaster);
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

		// step lights
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1]- 15, STD_ROWS8[STD_ROW1 + (s % 8)] - 19), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL6 : STD_COL2] , STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME::STEP_LIGHTS + s));
		}
		
		// step switches and pots
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addParam(createParamCentered<CountModulaToggle3P90>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1], STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: STEP_PARAMS + s));
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL7 : STD_COL3], STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: CV_PARAMS + s));
		}

		// determine where the final column of controls goes
		int lastCol = (SEQ_NUM_STEPS > 8 ? STD_COL9 : STD_COL5);
		
		// range control
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_HALF_ROWS8(STD_ROW5)), module, STRUCT_NAME::RANGE_SW_PARAM));
			
		// hold mode control
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_ROWS8[STD_ROW4]), module, STRUCT_NAME::HOLD_PARAM));
			
		// output lights and jacks
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_HALF_ROWS8(STD_ROW1)-20), module, STRUCT_NAME::GATE_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_HALF_ROWS8(STD_ROW1)), module, STRUCT_NAME::GATE_OUTPUT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_ROWS8[STD_ROW3]-20), module, STRUCT_NAME::TRIG_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_ROWS8[STD_ROW3]), module, STRUCT_NAME::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_ROWS8[STD_ROW7]), module, STRUCT_NAME::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[lastCol], STD_ROWS8[STD_ROW8]), module, STRUCT_NAME::CVI_OUTPUT));
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
				sprintf(buffer, "Channel %d (%s)", i, module->knobColours[i]);
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
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerInit && cvInit)
				h->name = "initialize cv";
			else if (triggerInit && !cvInit)
				h->name = "initialize gates/triggers";
			else
				h->name = "initialize cv/gates/trigs";

			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerInit)
					widget->getParam(STRUCT_NAME::STEP_PARAMS + c)->getParamQuantity()->reset();
				
				// cv knobs
				if (cvInit)
					widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->reset();
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

			// CV only init
			InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("CV Only");
			initCVMenuItem->widget = widget;
			initCVMenuItem->triggerInit = false;
			menu->addChild(initCVMenuItem);

			// gate/trigger only init
			InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Gates/Triggers Only");
			initTrigMenuItem->widget = widget;
			initTrigMenuItem->cvInit = false;
			menu->addChild(initTrigMenuItem);

			// CV/gate/trigger only init
			InitMenuItem *initCVTrigMenuItem = createMenuItem<InitMenuItem>("CV/Gates/Triggers Only");
			initCVTrigMenuItem->widget = widget;
			menu->addChild(initCVTrigMenuItem);

			return menu;	
		}
	};	
	
	// randomize selection menu item
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		bool triggerRand = true;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerRand && cvRand)
				h->name = "randomize CV";
			else if (triggerRand && !cvRand)
				h->name = "randomize gates/triggers";
			else
				h->name = "randomize cv/gates/trigs";
			
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerRand)
					widget->getParam(STRUCT_NAME::STEP_PARAMS + c)->getParamQuantity()->randomize();
				
				if (cvRand)
					widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
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

			// CV only random
			RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("CV Only");
			randCVMenuItem->widget = widget;
			randCVMenuItem->triggerRand = false;
			menu->addChild(randCVMenuItem);

			// gate/trigger only random
			RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Gates/Triggers Only");
			randTrigMenuItem->widget = widget;
			randTrigMenuItem->cvRand = false;
			menu->addChild(randTrigMenuItem);	
			
			// gate/trigger only random
			RandMenuItem *randCVTrigMenuItem = createMenuItem<RandMenuItem>("CV/Gates/Triggers Only");
			randCVTrigMenuItem->widget = widget;
			menu->addChild(randCVTrigMenuItem);	
			
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
				sprintf(buffer, "res/Components/Knob%s.svg", ((STRUCT_NAME*)module)->knobColours[cid]);
				
				for (int i = 0; i < SEQ_NUM_STEPS; i++) {
					ParamWidget *p = getParam(STRUCT_NAME::CV_PARAMS + i);
					((CountModulaKnob *)(p))->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, buffer))); 
					//((CountModulaKnob *)(p))->dirtyValue = -1;
				}
			}
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
