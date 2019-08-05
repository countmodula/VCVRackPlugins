//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Gated Comparator Module
//  A VCV rack implementation of the Ken Stone CGS13 Gated Comparator
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

struct GatedComparator : Module {

	enum ParamIds {
		THRESHOLD_PARAM,
		CV_PARAM,
		LOOP_EN_PARAM,
		ENUMS(MELODY_PARAMS, 8),
		NUM_PARAMS
	};
	
	enum InputIds {
		CLOCK_INPUT,
		COMP_INPUT,
		CV_INPUT,
		LOOP_INPUT,
		LOOP_EN_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		COMP_OUTPUT,
		RM_OUTPUT,
		RMI_OUTPUT,
		ENUMS(Q_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	
	enum LightIds {
		COMP_LIGHT,
		ENUMS(Q_LIGHTS, 8),
		LOOP_EN_LIGHT,
		NUM_LIGHTS
	};
	
	GateProcessor gpClock;
	GateProcessor gpLoopIn;
	GateProcessor gpLoopEnabled;
	short shiftReg = 0;
	bool loopEnabled = false;
	
	float stepValue = 8.0f / 255.0f;
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif	
	
	GatedComparator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(THRESHOLD_PARAM, -5.0f, 5.0f, 0.0f, "Comparator threshold", " V");
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "Comparator CV amount", " %", 0.0f, 100.0f, 0.0f);
		
		configParam(LOOP_EN_PARAM, 0.0f, 1.0, 0.0f, "Loop enable");
		
		// step params (knobs and switches)
		for (int s = 0; s < 8; s++) {
			configParam(MELODY_PARAMS + s, 0.0f, 1.0f, 0.0f, "Random melody");
		}
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
#endif			
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		json_object_set_new(root, "loopEnabled", json_boolean(loopEnabled));
		json_object_set_new(root, "shiftReg", json_integer(shiftReg));

		return root;
	}

	void dataFromJson(json_t *root) override {
		json_t *lpEn = json_object_get(root, "loopEnabled");
		json_t *shftRg = json_object_get(root, "shiftReg");
		
		if (lpEn)
			loopEnabled = json_boolean_value(lpEn);
		
		if (shftRg)
			shiftReg = json_integer_value(shftRg);
	}

	void onReset() override {
		gpClock.reset();
		gpLoopIn.reset();
		gpLoopEnabled.reset();
		loopEnabled = false;
		shiftReg = 0;
	}

	void process(const ProcessArgs &args) override {
		
		// Compute the threshold from the threshold parameter and cv input
		float threshold = params[THRESHOLD_PARAM].getValue();
		float cv = inputs[CV_INPUT].getVoltage() * params[CV_PARAM].getValue();
		
		// compare
		float compare = inputs[COMP_INPUT].getVoltage();
		bool state = (compare > (threshold + cv));

		// is loop enabled? - jack overrides button
		gpLoopEnabled.set(inputs[LOOP_EN_INPUT].getVoltage());
		if (inputs[LOOP_EN_INPUT].isConnected())
			loopEnabled = gpLoopEnabled.high();
		else
			loopEnabled = (params[LOOP_EN_PARAM].getValue() > 0.05f);
		
		// handle the loop input
		if (loopEnabled) {
			gpLoopIn.set(inputs[LOOP_INPUT].getVoltage());
			state |= gpLoopIn.high();
		}

		// process the clock
		float clock = inputs[CLOCK_INPUT].getVoltage(); 
		gpClock.set(clock);
		
		// shift the register if we need to
		if (gpClock.leadingEdge()) {
			shiftReg = shiftReg << 1;

			if(state)
				shiftReg |= 0x01;
		}
		
		// process the shift register values
		short bitMask = 0x01;
		float rm = 0.0f;
		for (int i = 0; i < 8; i ++) {
			// outputs and lights
			bool v = ((shiftReg & bitMask) == bitMask);
			outputs[Q_OUTPUTS + i].setVoltage(boolToGate(v));
			lights[Q_LIGHTS + i].setBrightness(boolToLight(v));
			
			// determine random melody value for this bit
			if (v && params[MELODY_PARAMS + i].getValue() > 0.05f)
				rm += (((float)(bitMask)) * stepValue);
			
			// prepare for next bit
			bitMask = bitMask << 1;
		}
		
		// other outputs and lights
		outputs[RM_OUTPUT].setVoltage(rm);
		outputs[RMI_OUTPUT].setVoltage(-rm);
		outputs[COMP_OUTPUT].setVoltage(boolToGate(state));
		lights[COMP_LIGHT].setBrightness(boolToLight(state));
		lights[LOOP_EN_LIGHT].setBrightness(boolToLight(loopEnabled));
		
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (rightExpander.module->model == modelSequencerExpanderCV8 || rightExpander.module->model == modelSequencerExpanderOut8 || 
				rightExpander.module->model == modelSequencerExpanderTrig8 || rightExpander.module->model == modelSequencerExpanderRM8) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set the expander module's channel number
				messageToExpander->setCVChannel(0);
				messageToExpander->setTrigChannel(0);
				messageToExpander->setOutChannel(0);
				messageToExpander->setRMChannel(0);
		
				// add the channel counters and gates
				int c = (int)(shiftReg & 0xFF);
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
					messageToExpander->counters[i] = shiftReg;
					messageToExpander->counters[i] = c;
					messageToExpander->clockStates[i] =	gpClock.high();
					messageToExpander->runningStates[i] = true; // always running - the counter takes care of the not running states 
				}
			
				// finally, let all subsequent expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_GTDCOMP;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif			
	}

};

struct GatedComparatorWidget : ModuleWidget {
	GatedComparatorWidget(GatedComparator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/GatedComparator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW3]), module, GatedComparator::CLOCK_INPUT));
		
		// Comparator section
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_HALF_ROWS6(STD_ROW1)), module, GatedComparator::COMP_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, GatedComparator::COMP_OUTPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, GatedComparator::COMP_INPUT));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, GatedComparator::THRESHOLD_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, GatedComparator::CV_INPUT));
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, GatedComparator::CV_PARAM));

		// loop and loop enable
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, GatedComparator::LOOP_INPUT));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_HALF_ROWS6(STD_ROW3)), module, GatedComparator:: LOOP_EN_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_HALF_ROWS6(STD_ROW3)), module, GatedComparator::LOOP_EN_LIGHT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_HALF_ROWS6(STD_ROW3)), module, GatedComparator::LOOP_EN_INPUT));

		// shift register section
		for (int s = 0; s < 8; s++) {
			int col = s > 3 ? STD_COL3 + ((s - 4) * 2) : STD_COL3 + (s * 2);
			int rowOffset = s > 3 ? 1 : 0;
			
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[col], STD_ROWS6[STD_ROW1 + rowOffset]), module, GatedComparator::Q_LIGHTS + s));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[col], STD_HALF_ROWS6(STD_ROW1  + rowOffset)), module, GatedComparator::Q_OUTPUTS + s));
		}

		// random melody section
		int col = STD_COLUMN_POSITIONS[STD_COL3];
		for (int s = 0; s < 8; s++) {
			addParam(createParamCentered<CountModulaToggle2P>(Vec(col, STD_ROWS6[STD_ROW5 + (s > 3 ? 1 : 0)]), module, GatedComparator::MELODY_PARAMS + s));
			
			if (s == 3)
				col = STD_COLUMN_POSITIONS[STD_COL3];
			else
				col += 40;
		}
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS6[STD_ROW5]), module, GatedComparator::RM_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS6[STD_ROW6]), module, GatedComparator::RMI_OUTPUT));
	}
	
	struct InitMenuItem : MenuItem {
		GatedComparatorWidget *widget;

		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize random melody";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			for (int c = 0; c < 8; c++) {
				widget->getParam(GatedComparator::MELODY_PARAMS + c)->reset();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		GatedComparatorWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize random melody";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			// step controls
			for (int c = 0; c < 8; c++) {
				widget->getParam(GatedComparator::MELODY_PARAMS + c)->randomize();
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
	
	void appendContextMenu(Menu *menu) override {
		GatedComparator *module = dynamic_cast<GatedComparator*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// pretty heading
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Gated Comparator";
		menu->addChild(settingsLabel);		

		// trigger only init
		InitMenuItem *initTrigMenuItem = createMenuItem<InitMenuItem>("Initialize Random Melody Only");
		initTrigMenuItem->widget = this;
		menu->addChild(initTrigMenuItem);

		// trigger only random
		RandMenuItem *randTrigMenuItem = createMenuItem<RandMenuItem>("Randomize Random Melody Only");
		randTrigMenuItem->widget = this;
		menu->addChild(randTrigMenuItem);			
	}	
};

Model *modelGatedComparator = createModel<GatedComparator, GatedComparatorWidget>("GatedComparator");
