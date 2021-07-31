//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Euclidean Sequencer CV Expander
//  Adds CV functionality to the Euclidean Sequencer module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/EuclidExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME EuclidExpanderCV
#define PANEL_FILE "EuclidExpanderCV.svg"

struct EuclidExpanderCV : Module {

	enum ParamIds {
		ENUMS(STEP_CV_PARAMS, EUCLID_EXP_NUM_STEPS),
		RANGE_SW_PARAM,
		CLOCK_SOURCE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		CV_OUTPUT,
		CVI_OUTPUT,
		GATE_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, EUCLID_EXP_NUM_STEPS),
		GATE_LIGHT,
		NUM_LIGHTS
	};
	
	enum triggerSources {
		CLOCK_RESTS,
		CLOCK_CLOCK,
		CLOCK_BEATS
	};
	
	// Expander details
	EuclidExpanderMessage leftMessages[2][1];	// messages from left module (master)
	EuclidExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	EuclidExpanderMessage *messagesFromMaster;
	
	bool leftModuleAvailable = false; 

	bool pulseOut;
	int triggerSource = -1;
	bool trig = false;
	bool running = false;
	bool clock = false;
	int count = -1;
	int prevCount = -1;
	
	int currentChannel = 0;
	int userChannel = 0;
	int prevChannel = 0;
	bool doRedraw = true;	
	
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
	
	EuclidExpanderCV() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
		
		// step params
		for (int s = 0; s < EUCLID_EXP_NUM_STEPS; s++) {
			configParam(STEP_CV_PARAMS + s, 0.0f, 8.0f, (float)(s + 1), "Step value");
		}
		
		// trigger source switch
		configParam(CLOCK_SOURCE_PARAM, 0.0f, 2.0f, 2.0f, "Clock Source");
			
		// range switch
		configParam(RANGE_SW_PARAM, 1.0f, 8.0f, 8.0f, "Scale");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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

		pulseOut = false;
		running = false;									
		count = -1;
		
		// where are we deriving our clock and trigger from (beats or rests)
		triggerSource = (int)(params[CLOCK_SOURCE_PARAM].getValue());
		
		// grab the detail from the left hand module if we have one
		currentChannel = 0;
		messagesFromMaster = 0;
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
					
				messagesFromMaster = (EuclidExpanderMessage *)(leftExpander.consumerMessage);

				// common ones
				trig = messagesFromMaster->trig;
				running = messagesFromMaster->running;
				clock = messagesFromMaster->clock;
				
				// Determine the output pulse and count to use
				switch (triggerSource) {
					case CLOCK_BEATS:
						pulseOut = messagesFromMaster->beatGate && clock;
						count = messagesFromMaster->beatCount;
						break;
					case CLOCK_RESTS:
						pulseOut = messagesFromMaster->restGate && clock;
						count = messagesFromMaster->restCount;
						break;
					case CLOCK_CLOCK:
						pulseOut = clock && running;
						count = messagesFromMaster->stepCount;
					break;
				}
				
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

		// set the pulse output to the width for the clock as this is not available on the main module
		pulseOut &= running;
		
		// determine which scale to use
		float scale = params[RANGE_SW_PARAM].getValue() / 8.0f;
	
		// now deal with the CV and lights
		float cv = 0.0f;
		if (running) {
			for (int c = 0; c < EUCLID_EXP_NUM_STEPS; c++) {

				if (c == count) {
					cv = params[STEP_CV_PARAMS + c].getValue() * scale;

					// set step lights here
					lights[STEP_LIGHTS + c].setBrightness(1.0f);
				}
				else {
						lights[STEP_LIGHTS + c].setBrightness(0.0f);
				}
			}
		}
		
		// set the outputs accordingly
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
		
		outputs[GATE_OUTPUT].setVoltage(boolToGate(pulseOut));
		lights[GATE_LIGHT].setBrightness(boolToGate(pulseOut));

		prevCount = count;
		
		// finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				EuclidExpanderMessage *messageToExpander = (EuclidExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// just pass the master module details through
				if (messagesFromMaster) {
						int ch = 0;
					if (messagesFromMaster->hasMaster) {
						ch = messagesFromMaster->channel;

						if (++ch > 7)
							ch = 1;
					}
					
					messageToExpander->set(messagesFromMaster->beatGate,
											messagesFromMaster->restGate, 
											messagesFromMaster->clockEdge, 
											messagesFromMaster->clock, 
											messagesFromMaster->trig, 
											messagesFromMaster->running, 
											messagesFromMaster->beatCount, 
											messagesFromMaster->restCount, 
											messagesFromMaster->stepCount, 
											ch, 
											messagesFromMaster->hasMaster);
				}
				else {
					messageToExpander->initialise();
				}
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct EuclidExpanderCVWidget : ModuleWidget {

	std::string panelName;
	
	EuclidExpanderCVWidget(EuclidExpanderCV *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// row lights and knobs
		for (int s = 0; s < EUCLID_EXP_NUM_STEPS; s++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW1 + s]), module, EuclidExpanderCV::STEP_LIGHTS + s));
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + s]), module, EuclidExpanderCV::STEP_CV_PARAMS + s));
		}

		// source and range controls
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW1)), module, EuclidExpanderCV::CLOCK_SOURCE_PARAM));
		addParam(createParamCentered<RotarySwitch<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW5)), module, EuclidExpanderCV::RANGE_SW_PARAM));
		
		// gate output/light
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW3) - 19), module, EuclidExpanderCV::GATE_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW3)), module, EuclidExpanderCV::GATE_OUTPUT));
	
		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, EuclidExpanderCV::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, EuclidExpanderCV::CVI_OUTPUT));
	}
	
	// Channel selection menu item
	struct ChannelMenuItem : MenuItem {
		EuclidExpanderCV *module;
		int channelToUse = 0;
		
		void onAction(const event::Action &e) override {
			module->userChannel = channelToUse;
		}
	};	
		
	// channel menu item
	struct ChannelMenu : MenuItem {
		EuclidExpanderCV *module;		
		
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
	
	struct ZeroMenuItem : MenuItem {
		EuclidExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "zero cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < EUCLID_EXP_NUM_STEPS; i ++)
				widget->getParam(EuclidExpanderCV::STEP_CV_PARAMS + i)->paramQuantity->setValue(0.0f);

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};		
	
	struct InitMenuItem : MenuItem {
		EuclidExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			for (int i = 0; i < EUCLID_EXP_NUM_STEPS; i ++)
				widget->getParam(EuclidExpanderCV::STEP_CV_PARAMS + i)->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		EuclidExpanderCVWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize cv";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			for (int i = 0; i < EUCLID_EXP_NUM_STEPS; i ++)
				widget->getParam(EuclidExpanderCV::STEP_CV_PARAMS + i)->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	

	// include the theme menu item struct we'll use when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"							
		
	void appendContextMenu(Menu *menu) override {
		EuclidExpanderCV *module = dynamic_cast<EuclidExpanderCV*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
				
		// add channel menu item
		ChannelMenu *channelMenuItem = createMenuItem<ChannelMenu>("Channel", RIGHT_ARROW);
		channelMenuItem->module = module;
		menu->addChild(channelMenuItem);		
		
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
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"

			if (((EuclidExpanderCV*)module)->doRedraw) {
				int cid = ((EuclidExpanderCV*)module)->currentChannel;
				char buffer[50];
				sprintf(buffer, "res/Components/Knob%s.svg", ((EuclidExpanderCV*)module)->knobColours[cid]);
				
				for (int i = 0; i < EUCLID_EXP_NUM_STEPS; i++) {
					ParamWidget *p = getParam(EuclidExpanderCV::STEP_CV_PARAMS + i);
					((CountModulaKnob *)(p))->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, buffer))); 
					((CountModulaKnob *)(p))->dirtyValue = -1;
				}
			}			
		}
		
		Widget::step();
	}	
};

Model *modelEuclidExpanderCV = createModel<EuclidExpanderCV, EuclidExpanderCVWidget>("EuclidExpanderCV");
