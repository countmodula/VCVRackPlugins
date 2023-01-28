//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard Sequencer Channel Engine
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, SEQ_NUM_STEPS),
		ENUMS(CV_PARAMS, SEQ_NUM_STEPS),
		RANGE_SW_PARAM,
		HOLD_PARAM,		
		ENUMS(TRIGGER_PARAMS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAMS, SEQ_NUM_STEPS),
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
		ENUMS(TRIGGER_PARAM_LIGHTS, SEQ_NUM_STEPS),
		ENUMS(GATE_PARAM_LIGHTS, SEQ_NUM_STEPS),
		NUM_LIGHTS
	};
	
	int count;
	int length;
	int currentChannel = 0;
	int userChannel = 0;
	int prevChannel = 0;
	bool doRedraw = true;
	
	int moduleVersion = 2;
	
	float cv = 0.0f;
	bool prevGate = false;

	// Expander details
	SequencerChannelMessage leftMessages[2][1];	// messages from left module (master)
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerChannelMessage *messagesFromMaster;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// cv/gate params
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configSwitch(STEP_PARAMS + s, 0.0f, 2.0f, 1.0f, rack::string::f("Step %d select", s + 1), {"Gate", "Off", "Trigger"});
			configSwitch(TRIGGER_PARAMS + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d trigger select", s + 1), {"Off", "On"});
			configSwitch(GATE_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d gate select", s + 1), {"Off", "On"});
			configParam(CV_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d value", s + 1), " V", 0.0f, 8.0f, 0.0f);
		}

		// range switch
		configSwitch(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale", {"1 Volt", "2 Volts", "3 Volts", "4 Volts", "5 Volts", "6 Volts", "7 Volts", "8 Volts"} );

		// hold mode switch
		configSwitch(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Sample and hold mode", {"Trigger", "Off", "Gate"});

		configOutput(GATE_OUTPUT, "Gate");
		configOutput(TRIG_OUTPUT, "Trigger");
		configOutput(CV_OUTPUT, "CV");
		configOutput(CVI_OUTPUT, "Inverted CV");

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

		json_object_set_new(root, "moduleVersion", json_integer(moduleVersion));
		json_object_set_new(root, "channel", json_integer(userChannel));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
		
		json_t *ch = json_object_get(root, "channel");
		json_t *version = json_object_get(root, "moduleVersion");
		
		if (ch)
			userChannel = json_integer_value(ch);				
	
		if (version)
			moduleVersion = json_integer_value(version);
		else {
			// no version in file is really old version.
			moduleVersion = 0;
		}
	
		// conversion to new step select switches
		if (moduleVersion < 2) {
			INFO("Converting from module version %d", moduleVersion);

			for(int i = 0; i < SEQ_NUM_STEPS; i++) {
				int x = (int)(params[STEP_PARAMS + i].getValue());

				params[GATE_PARAMS + i].setValue(x == 0 ? 1.0f : 0.0f);
				params[TRIGGER_PARAMS + i].setValue(x == 2 ? 1.0f : 0.0f);
			}
			
			moduleVersion = 2;
		}	
	
	
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
				
				// for the binary addressed sequencer, we need to chop off the 4th bit if we're running an 8 step channel
				if (count > SEQ_NUM_STEPS)
					count -= SEQ_NUM_STEPS;

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
				if(params[TRIGGER_PARAMS + c].getValue() > 0.5f) {
					trig = running;
				}
				
				if(params[GATE_PARAMS + c].getValue() > 0.5f) {
					gate = running;
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

		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	

		// step lights
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1] + 5, STD_ROWS8[STD_ROW1 + (s % 8)] - 8), module, STRUCT_NAME::LENGTH_LIGHTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1] + 5 , STD_ROWS8[STD_ROW1 + (s % 8)] + 5), module, STRUCT_NAME::STEP_LIGHTS + s));
		}
		
		// step switches and pots
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1] - 15, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: TRIGGER_PARAMS + s, STRUCT_NAME:: TRIGGER_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[s > 7 ? STD_COL5 : STD_COL1] + 25, STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME:: GATE_PARAMS + s, STRUCT_NAME:: GATE_PARAM_LIGHTS + s));

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
		int which;
		
		std::string undoLabels[4] = {"initialize cv", "initialize gates", "initialize triggers", "initialize cv/gates/triggers"};
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = undoLabels[which];
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			switch (which) {
				case 0: // cv only
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->reset();
					}
					break;					
				case 1: // gates only
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->reset();
					}
					break;				
				case 2: // triggers
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->reset();
					}
					break;				
				case 3: // cv/gates/triggers
					for (int c = 0; c < SEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->reset();
						widget->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->reset();
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->reset();
					}
					break;					
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	// initialize menu item
	struct InitMenu : MenuItem {
		WIDGET_NAME *widget;		
		std::string initLabels[4] = {"CV only", "Gates only", "Triggers only", "CV/Gates/Triggers only"};
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < 4; i++) {
				InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>(initLabels[i]);
				initMenuItem->widget = widget;
				initMenuItem->which = i;
				menu->addChild(initMenuItem);
			}

			return menu;	
		}
	};	
	
	
	std::string undoRandLabels[4] = {"randomize cv", "randomize gates", "randomize triggers", "randomize cv/gates/triggers"};
	
	void doRandom(int which) {
		// history - current settings
		history::ModuleChange *h = new history::ModuleChange;
		h->name = undoRandLabels[which];
		h->moduleId = this->module->id;
		h->oldModuleJ = this->toJson();

		// step controls
		switch (which) {
			case 0: // cv only
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;					
			case 1: // gates only
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;				
			case 2: // triggers
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;				
			case 3: // cv/gates/triggers
				for (int c = 0; c < SEQ_NUM_STEPS; c++) {
					this->getParam(STRUCT_NAME::CV_PARAMS + c)->getParamQuantity()->randomize();
					this->getParam(STRUCT_NAME::GATE_PARAMS + c)->getParamQuantity()->randomize();
					this->getParam(STRUCT_NAME::TRIGGER_PARAMS + c)->getParamQuantity()->randomize();
				}
				break;					
		}

		// history - new settings
		h->newModuleJ = this->toJson();
		APP->history->push(h);	
	}

	// randomize selection menu item
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int which;

		void onAction(const event::Action &e) override {
			widget->doRandom(which);
		}
	};
	
	// randomize menu item
	struct RandMenu : MenuItem {
		WIDGET_NAME *widget;
		std::string randLabels[4] = {"CV only", "Gates only", "Triggers only", "CV/Gates/Triggers only"};
		std::string randKeys[4] = {"Shitf+Ctrl+C", "Shitf+Ctrl+G", "Shitf+Ctrl+T", "Shitf+Ctrl+R"};

		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			for (int i = 0;i < 4; i++) {
				// CV only random
				RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>(randLabels[i], randKeys[i]);
				randMenuItem->widget = widget;
				randMenuItem->which = i;
				menu->addChild(randMenuItem);
			}
		
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
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));
		
		// add channel menu item
		ChannelMenu *channelMenuItem = createMenuItem<ChannelMenu>("Channel", RIGHT_ARROW);
		channelMenuItem->module = module;
		menu->addChild(channelMenuItem);
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));		
		
		// add initialize menu item
		InitMenu *initMenuItem = createMenuItem<InitMenu>("Initialize", RIGHT_ARROW);
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// add randomize menu item
		RandMenu *randMenuItem = createMenuItem<RandMenu>("Randomize", RIGHT_ARROW);
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);
	}

	void onHoverKey(const event::HoverKey &e) override {
		if (e.action == GLFW_PRESS && (e.mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			
			switch (e.key) {
				case GLFW_KEY_C:
					doRandom(0);
					e.consume(this);
					break;
				case GLFW_KEY_G:
					doRandom(1);
					e.consume(this);
					break;						
				case GLFW_KEY_T:
					doRandom(2);
					e.consume(this);
					break;	
				case GLFW_KEY_R:
					doRandom(3);
					e.consume(this);
					break;				
			}
		}

		ModuleWidget::onHoverKey(e);
	}
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"			
	
			if (((STRUCT_NAME*)module)->doRedraw) {
				int cid = ((STRUCT_NAME*)module)->currentChannel;
				
				char buffer[50];
				sprintf(buffer, "res/Components/Knob%s.svg", CountModulaknobColours[cid]);
				
				for (int i = 0; i < SEQ_NUM_STEPS; i++) {
					CountModulaKnob *p = (CountModulaKnob *)getParam(STRUCT_NAME::CV_PARAMS + i);
					p->svgFile = CountModulaknobColours[cid];
					p->setSvg(Svg::load(asset::plugin(pluginInstance, buffer))); 
					p->fb->dirty = true;
				}
			}
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
