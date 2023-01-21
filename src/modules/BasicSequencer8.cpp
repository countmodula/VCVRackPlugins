//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//	A classic 8 step CV/Gate sequencer
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define SEQ_NUM_STEPS	8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BasicSequencer8
#define PANEL_FILE "BasicSequencer8.svg"

struct BasicSequencer8 : Module {

	enum ParamIds {
		ENUMS(STEP_SW_PARAMS, SEQ_NUM_STEPS),
		ENUMS(STEP_CV_PARAMS, SEQ_NUM_STEPS),
		LENGTH_PARAM,
		CV_PARAM,
		RANGE_SW_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		RUN_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		LENCV_INPUT,
		DIRCV_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		TRIG_OUTPUT,
		GATE_OUTPUT,
		CV_OUTPUT,
		CVI_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		TRIG_LIGHT,
		GATE_LIGHT,
		ENUMS(DIR_LIGHTS, 3),
		ENUMS(LENGTH_LIGHTS, SEQ_NUM_STEPS),
		NUM_LIGHTS
	};
	
	enum Directions {
		FORWARD,
		PENDULUM,
		REVERSE
	};
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	dsp::PulseGenerator pgClock;
	
	int count = 0;
	int length = 8;
	int direction = 0;
	int directionMode = 0;
	bool running = false;
	
	float lengthCVScale = (float)(SEQ_NUM_STEPS - 1);
	float moduleVersion = 1.4f;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif

	BasicSequencer8() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length param
		configSwitch(LENGTH_PARAM, 1.0f, (float)(SEQ_NUM_STEPS), (float)(SEQ_NUM_STEPS), "Sequence length", {"1 step", "2 steps", "3 steps", "4 steps", "5 steps", "6 steps", "7 steps", "8 steps"});
		
		//  mode param
		configSwitch(MODE_PARAM, 0.0f, 2.0, 0.0f, "Direction", {"Down", "Bi", "Up"});
		
		// step params (knobs and switches)
		std::ostringstream  buffer;
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			configSwitch(STEP_SW_PARAMS + s, 0.0f, 2.0f, 1.0f, "Step", {"Gate", "Off", "Trigger"});
			configParam(STEP_CV_PARAMS + s, 0.0f, 8.0f, 0.0f, "Step value");
			buffer.str("");
			buffer << "Length = " << s + 1;
			configLight(LENGTH_LIGHTS + s, buffer.str());
			buffer.str("");
			buffer << "Step " << s +1 << " active";
			configLight(STEP_LIGHTS + s, buffer.str());
		}
		
		// range switch
		configSwitch(RANGE_SW_PARAM, 0.0f, 2.0f, 0.0f, "Scale", {"8V", "4V", "2V"});
		
		configInput(RUN_INPUT, "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENCV_INPUT, "Length CV");
		configInput(DIRCV_INPUT, "Direction CV");
		
		configOutput(TRIG_OUTPUT, "Trigger");
		configOutput(GATE_OUTPUT, "Gate");
		configOutput(CV_OUTPUT, "CV");
		configOutput(CVI_OUTPUT, "Inverted CV");
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
#endif

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(moduleVersion));
		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "direction", json_integer(direction));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"
				
		return root;
	}

	void dataFromJson(json_t *root) override {
		
		json_t *version = json_object_get(root, "moduleVersion");
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *dir = json_object_get(root, "direction");
		json_t *run = json_object_get(root, "runState");		
		
		if (version)
			moduleVersion = json_number_value(version);		
		
		if (currentStep)
			count = json_integer_value(currentStep);
		
		if (dir)
			direction = json_integer_value(dir);

		if (run) 
			gateRun.preset(json_boolean_value(run));
		
		running = gateRun.high();
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}

	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		pgClock.reset();
		count = 0;
		length = SEQ_NUM_STEPS;
		direction = FORWARD;
		directionMode = FORWARD;
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
	
	int recalcDirection() {
		
		switch (directionMode) {
			case PENDULUM:
				return (direction == FORWARD ? REVERSE : FORWARD);
			case FORWARD:
			case REVERSE:
			default:
				return directionMode;
		}
	}
	
	void setDirectionLight() {
		lights[DIR_LIGHTS].setBrightness(boolToLight(directionMode == REVERSE)); 		// red
		lights[DIR_LIGHTS + 1].setBrightness(boolToLight(directionMode == PENDULUM) * 0.5f); 	// yellow
		lights[DIR_LIGHTS + 2].setBrightness(boolToLight(directionMode == FORWARD)); 	// green
	}
	
	void process(const ProcessArgs &args) override {

		// grab all the common input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;

		// reset input
		f = inputs[RESET_INPUT].getNormalVoltage(reset);
		gateReset.set(f);
		reset = f;
		
		// run input
		f = inputs[RUN_INPUT].getNormalVoltage(run);
		gateRun.set(f);
		run = f;

		// clock
		f = inputs[CLOCK_INPUT].getNormalVoltage(clock); 
		gateClock.set(f);
		clock = f;
		
		// sequence length - jack overrides knob
		if (inputs[LENCV_INPUT].isConnected()) {
			// scale the input such that 10V = step 16, 0V = step 1
			length = (int)(clamp(lengthCVScale/10.0f * inputs[LENCV_INPUT].getVoltage(), 0.0f, lengthCVScale)) + 1;
		}
		else {
			length = (int)(params[LENGTH_PARAM].getValue());
		}
		
		// set the length lights
		for(int i = 0; i < SEQ_NUM_STEPS; i++) {
			lights[LENGTH_LIGHTS +  i].setBrightness(boolToLight(i < length));
		}
			
		// direction - jack overrides the switch
		if (inputs[DIRCV_INPUT].isConnected()) {
			float dirCV = clamp(inputs[DIRCV_INPUT].getVoltage(), 0.0f, 10.0f);
			if (dirCV < 2.0f)
				directionMode = FORWARD;
			else if (dirCV < 4.0f)
				directionMode = PENDULUM;
			else
				directionMode = REVERSE;
		}
		else
			directionMode = (int)(params[MODE_PARAM].getValue());

		setDirectionLight();			
	
		// which mode are we using? jack overrides the switch
		int nextDir = recalcDirection();
		
		// switch non-pendulum mode right away
		if (directionMode != PENDULUM)
			direction = nextDir;
		
		if (gateReset.leadingEdge()) {
			
			// restart pendulum at forward stage
			if (directionMode == PENDULUM)
				direction = nextDir = FORWARD;
			
			// reset the count according to the next direction
			switch (nextDir) {
				case FORWARD:
					count = 0;
					break;
				case REVERSE:
					count = SEQ_NUM_STEPS + 1;
					break;
			}
			
			direction = nextDir;
		}

		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		bool clockEdge = gateClock.leadingEdge();
		if (clockEdge)
			pgClock.trigger(1e-4f);
		else if (pgClock.process(args.sampleTime)) {
			// if within cooey of the clock edge, run or reset is treated as a clock edge.
			clockEdge = (gateRun.leadingEdge() || gateReset.leadingEdge());
		}
		
		if (gateRun.low())
			running = false;
		
		// advance count on positive clock edge or the run edge if it is close to the clock edge
		if (clockEdge && gateRun.high()) {
			
			// flag that we are now actually running
			running = true;
			
			if (direction == FORWARD) {
				count++;
				
				if (count > length) {
					if (nextDir == FORWARD)
						count = 1;
					else {
						// in pendulum mode we change direction here
						count--;
						direction = nextDir;
					}
				}
			}
			else {
				count--;
				
				if (count < 1) {
					if (nextDir == REVERSE)
						count = length;
					else {
						// in pendulum mode we change direction here
						count++;
						direction = nextDir;
					}
				}
			}
			
			if (count > length)
				count = length;				
		}
		
		// now process the lights and outputs
		bool trig = false;
		bool gate = false;
		float cv = 0.0f;
		float scale = 1.0f;
		
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// set step lights here
			bool stepActive = ((c + 1) == count);
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));
			
			// now determine the output values
			if (stepActive) {
				// trigger/gate
				switch ((int)(params[STEP_SW_PARAMS + c].getValue())) {
					case 0: // lower output
						trig = false;
						gate = true;
						break;
					case 2: // upper output
						trig = true;
						gate = false;
						break;				
					default: // off
						trig = false;
						gate = false;
						break;
				}
					
				// determine which scale to use
				scale = getScale(params[RANGE_SW_PARAM].getValue());
				
				// now grab the cv value
				cv = params[STEP_CV_PARAMS + c].getValue() * scale;
			}
		}

		// trig output follows clock width
		trig &= (running && gateClock.high());

		// gate output follows step widths
		gate &= running;

		// set the outputs accordingly
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig));	
		outputs[GATE_OUTPUT].setVoltage(boolToGate(gate));	
		
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[CVI_OUTPUT].setVoltage(-cv);
	
		lights[TRIG_LIGHT].setBrightness(boolToLight(trig));	
		lights[GATE_LIGHT].setBrightness(boolToLight(gate));
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set any potential expander module's channel number
				messageToExpander->setAllChannels(0);
	
				// add the channel counters and gates
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
					messageToExpander->counters[i] = count;
					messageToExpander->clockStates[i] =	gateClock.high();
					messageToExpander->runningStates[i] = running;
				}
				
				// finally, let all potential expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_BASIC;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif		
	}

};

struct BasicSequencer8Widget : ModuleWidget {

	std::string panelName;
	
	BasicSequencer8Widget(BasicSequencer8 *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// run input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, BasicSequencer8::RUN_INPUT));

		// reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, BasicSequencer8::RESET_INPUT));
		
		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, BasicSequencer8::CLOCK_INPUT));
				
		// length CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, BasicSequencer8::LENCV_INPUT));

		// direction CV input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1]), module, BasicSequencer8::DIRCV_INPUT));
		
		// direction light
		addChild(createLightCentered<MediumLight<CountModulaLightRYG>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 5, STD_HALF_ROWS8(STD_ROW1) - 5), module, BasicSequencer8::DIR_LIGHTS));
			
		// length & mode params
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW2]), module, BasicSequencer8::MODE_PARAM));
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW3)), module, BasicSequencer8::LENGTH_PARAM));
			
		// row lights and switches
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] - 15, STD_ROWS8[STD_ROW1 + s] - 19), module, BasicSequencer8::LENGTH_LIGHTS + s));
			addParam(createParamCentered<CountModulaToggle3P90>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8:: STEP_SW_PARAMS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8::STEP_LIGHTS + s));
			addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + s]), module, BasicSequencer8::STEP_CV_PARAMS + s));
		}
			
		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 20), module, BasicSequencer8::TRIG_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8] - 20), module, BasicSequencer8::GATE_LIGHT));
			
		// range controls
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW5]), module, BasicSequencer8::RANGE_SW_PARAM));

		// cv outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW6)), module, BasicSequencer8::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_HALF_ROWS8(STD_ROW6)), module, BasicSequencer8::CVI_OUTPUT));
		
		// gate/trig output jack
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, BasicSequencer8::TRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8]), module, BasicSequencer8::GATE_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	struct InitMenuItem : MenuItem {
		BasicSequencer8Widget *widget;
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {

		// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			if (!triggerInit && cvInit)
				h->name = "initialize CV";
			else if (triggerInit && !cvInit)
				h->name = "initialize triggers";
			else
				h->name = "initialize module";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			for (int c = 0; c < SEQ_NUM_STEPS; c++) {
				// triggers/gates
				if (triggerInit)
					widget->getParam(BasicSequencer8::STEP_SW_PARAMS + c)->getParamQuantity()->reset();
				
				// cv knobs
				if (cvInit)
					widget->getParam(BasicSequencer8::STEP_CV_PARAMS + c)->getParamQuantity()->reset();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	
	void doRandom(bool triggerRand, bool cvRand){
		// history - current settings
		history::ModuleChange *h = new history::ModuleChange;
		if (!triggerRand && cvRand)
			h->name = "randomize cv";
		else if (triggerRand && !cvRand)
			h->name = "randomize gates/triggers";
		else
			h->name = "randomize cv/gates/triggers";
		
		h->moduleId = this->module->id;
		h->oldModuleJ = this->toJson();

		// step controls
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// triggers/gates
			if (triggerRand)
				this->getParam(BasicSequencer8::STEP_SW_PARAMS + c)->getParamQuantity()->randomize();
			
			if (cvRand)
				this->getParam(BasicSequencer8::STEP_CV_PARAMS + c)->getParamQuantity()->randomize();
		}

		// history - new settings
		h->newModuleJ = this->toJson();
		APP->history->push(h);	
	}	
	
	struct RandMenuItem : MenuItem {
		BasicSequencer8Widget *widget;
		bool triggerRand = true;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
			widget->doRandom(triggerRand, cvRand);
		}
	};
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS		
	// expander addition menu item
	#include "../inc/AddExpanderMenuItem.hpp"	
#endif

	void appendContextMenu(Menu *menu) override {
		BasicSequencer8 *module = dynamic_cast<BasicSequencer8*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));		
		// CV only init
		InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("Initialize CV Only");
		initCVMenuItem->widget = this;
		initCVMenuItem->triggerInit = false;
		menu->addChild(initCVMenuItem);

		// trigger only init
		InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Initialize Gates/Triggers Only");
		initTrigMenuItem->widget = this;
		initTrigMenuItem->cvInit = false;
		menu->addChild(initTrigMenuItem);

		// trigger only init
		InitMenuItem *initCVTrigMenuItem = createMenuItem<InitMenuItem>("Initialize CV/Gates/Triggers Only");
		initCVTrigMenuItem->widget = this;
		menu->addChild(initCVTrigMenuItem);

		// CV only random
		RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("Randomize CV Only", "Shift+Ctrl+C");
		randCVMenuItem->widget = this;
		randCVMenuItem->triggerRand = false;
		menu->addChild(randCVMenuItem);

		// gate/trigger only random
		RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Randomize Gates/Triggers Only", "Shift+Ctrl+T");
		randTrigMenuItem->widget = this;
		randTrigMenuItem->cvRand = false;
		menu->addChild(randTrigMenuItem);

		// cv/gate/trigger only random
		RandMenuItem *randCVTrigMenuItem = createMenuItem<RandMenuItem>("Randomize CV/Gates/Triggers Only", "Shift+Ctrl+R");
		randCVTrigMenuItem->widget = this;
		menu->addChild(randCVTrigMenuItem);
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS			
		// add expander menu
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Expansion"));
	
		AddExpanderMenuItem *cvMenuItem = createMenuItem<AddExpanderMenuItem>("Add CV expander");
		cvMenuItem->module = module;
		cvMenuItem->model = modelSequencerExpanderCV8;
		cvMenuItem->position = box.pos;
		cvMenuItem->expanderName = "CV";
		menu->addChild(cvMenuItem);	
		
		AddExpanderMenuItem *outputMenuItem = createMenuItem<AddExpanderMenuItem>("Add output expander");
		outputMenuItem->module = module;
		outputMenuItem->model = modelSequencerExpanderOut8;
		outputMenuItem->position = box.pos;
		outputMenuItem->expanderName = "output";
		menu->addChild(outputMenuItem);		
		
		AddExpanderMenuItem *trigMenuItem = createMenuItem<AddExpanderMenuItem>("Add trigger expander");
		trigMenuItem->module = module;
		trigMenuItem->model = modelSequencerExpanderTrig8;
		trigMenuItem->position = box.pos;
		trigMenuItem->expanderName = "trigger";
		menu->addChild(trigMenuItem);			
		
		AddExpanderMenuItem *melodyMenuItem = createMenuItem<AddExpanderMenuItem>("Add random melody expander");
		melodyMenuItem->module = module;
		melodyMenuItem->model = modelSequencerExpanderRM8;
		melodyMenuItem->position = box.pos;
		melodyMenuItem->expanderName = "random melody";
		menu->addChild(melodyMenuItem);	

		AddExpanderMenuItem *logicMenuItem = createMenuItem<AddExpanderMenuItem>("Add logic expander");
		logicMenuItem->module = module;
		logicMenuItem->model = modelSequencerExpanderLog8;
		logicMenuItem->position = box.pos;
		logicMenuItem->expanderName = "logic";
		menu->addChild(logicMenuItem);			
#endif		
	}

	void onHoverKey(const event::HoverKey &e) override {
		if (e.action == GLFW_PRESS && (e.mods & RACK_MOD_MASK) == (RACK_MOD_CTRL | GLFW_MOD_SHIFT)) {
			
			switch (e.key) {
				case GLFW_KEY_C:
					doRandom(false, true);
					e.consume(this);
					break;
				case GLFW_KEY_R:
					doRandom(true, true);
					e.consume(this);
					break;
				case GLFW_KEY_T:
					doRandom(true, false);
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
		}
		
		Widget::step();
	}		
};

Model *modelBasicSequencer8 = createModel<BasicSequencer8, BasicSequencer8Widget>("BasicSequencer8");
