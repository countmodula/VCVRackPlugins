//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Binary Sequencer Module
//	VCV Rack version of now extinct Blacet Binary Zone Frac Module.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockOscillator.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

struct BinarySequencer : Module {
	enum ParamIds {
		DIV01_PARAM,
		DIV02_PARAM,
		DIV04_PARAM,
		DIV08_PARAM,
		DIV16_PARAM,
		DIV32_PARAM,
		SCALE_PARAM,
		CLOCKRATE_PARAM,
		LAG_PARAM,
		LAGSHAPE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		RUN_INPUT,
		SH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		INV_OUTPUT,
		CLOCK_OUTPUT,
		TRIGGER_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		NUM_LIGHTS
	};

	int counter = 0;

	float clockFreq = 1.0f;
	float scale = 0.0f;
	
	GateProcessor gateClock;
	GateProcessor gateRun;
	GateProcessor gateReset;
	dsp::PulseGenerator  pgTrig;
	ClockOscillator clock;
	LagProcessor slew;

#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif
	
	BinarySequencer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// CV knobs
		configParam(DIV01_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 2");
		configParam(DIV02_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 4");
		configParam(DIV04_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 8");
		configParam(DIV08_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 16");
		configParam(DIV16_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 32");
		configParam(DIV32_PARAM, -10.0f, 10.0f, 0.0f,"Divide by 64");
		
		// other knobs
		configParam(CLOCKRATE_PARAM, -3.0f, 7.0f, 1.0f, "Clock Rate");
		configParam(LAG_PARAM, 0.0f, 1.0f, 0.0f, "Lag Amount");
		configParam(LAGSHAPE_PARAM, 0.0f, 1.0f, 0.0f, "Lag Shape");
	
		// scale switch
		configParam(SCALE_PARAM, 0.0f, 2.0f, 0.0f, "Scale");
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
#endif		
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.3"));
		
		return root;
	}	
	
	void process(const ProcessArgs &args) override {

		// generate the internal clock value
		clock.setPitch(params[CLOCKRATE_PARAM].getValue());
		clock.step(args.sampleTime);
		
		// handle the run input
		gateRun.set(inputs[RUN_INPUT].getNormalVoltage(10.0f));
		
		// handle the reset input
		if (inputs[RESET_INPUT].isConnected()) {
			// reset really is reset
			gateReset.set(inputs[RESET_INPUT].getVoltage());
		}
		else {
			// reset is derived from the leading edge of the run input
			if (gateRun.leadingEdge())
				gateReset.set(10.0f);
			else
				gateReset.set(0.0f);
		}
		
		// reset on reset input 
		if (gateReset.leadingEdge())
			counter = 0;
		
		// grab the clock input value
		float internalClock = 5.0f * clock.sqr();
		float clockState = inputs[CLOCK_INPUT].getNormalVoltage(internalClock);
		gateClock.set(clockState);
		
		if (gateRun.high()) {
			// is it a transition from low to high?
			if (gateClock.leadingEdge()) {
				
				// kick off the trigger
				pgTrig.trigger(1e-3f);

				if (++counter > 255)
					counter = 0;
			}
		}
		
		// determine current scale
		if (inputs[SH_INPUT].isConnected()) {
			// perform the S&H function if required
			if (gateClock.leadingEdge())
				scale = clamp(inputs[SH_INPUT].getNormalVoltage(0.0f), -10.0f, 10.0f) / 10.0f / 6.0f;
		}
		else {
			switch ((int)(params[SCALE_PARAM].getValue()))
			{
				case 0: // +/- 10V
					scale = 1.0f / 6.0f;
					break;
				case 1: // +/- 5V
					scale = 1.0f / 6.0f / 2.0f;
					break;
				case 2: // +/- 2V
				default:
					scale = 1.0f / 6.0f / 5.0f;			
					break;
			}
		}

		// calculate the CV value
		float cv = 0.0f;
		if (gateRun.high())
		{
			if ((counter & 0x01) == 0x01) cv += params[DIV01_PARAM].getValue();
			if ((counter & 0x02) == 0x02) cv += params[DIV02_PARAM].getValue();
			if ((counter & 0x04) == 0x04) cv += params[DIV04_PARAM].getValue();
			if ((counter & 0x08) == 0x08) cv += params[DIV08_PARAM].getValue();
			if ((counter & 0x10) == 0x10) cv += params[DIV16_PARAM].getValue();
			if ((counter & 0x20) == 0x20) cv += params[DIV32_PARAM].getValue();
		}
		
		// scale the output to the currently selected value
		cv = clamp(cv * scale, -10.0f, 10.0f);
		
		// apply lag
		cv = slew.process(cv, params[LAGSHAPE_PARAM].getValue(), params[LAG_PARAM].getValue(), params[LAG_PARAM].getValue());
		
		// set the outputs
		outputs[CV_OUTPUT].setVoltage(cv);
		outputs[INV_OUTPUT].setVoltage(-cv);
		outputs[CLOCK_OUTPUT].setVoltage(gateClock.value());
		outputs[TRIGGER_OUTPUT].setVoltage(boolToGate(pgTrig.process(args.sampleTime)));
		
		// blink the light according to the clock
		lights[CLOCK_LIGHT].setSmoothBrightness(gateClock.light(), args.sampleTime);
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set any potential expander module's channel number
				messageToExpander->setAllChannels(0);
	
				// add the channel counters and gates
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
					messageToExpander->counters[i] = counter;
					messageToExpander->clockStates[i] =	gateClock.high();
					messageToExpander->runningStates[i] = gateRun.high();
				}		
				
				// finally, let all potential expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_BINARY;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif	
	}
	
	void onReset() override {
		gateClock.reset();
		gateRun.reset();
		gateReset.reset();
		pgTrig.reset();
		clock.reset();
		slew.reset();
		scale = 0.0f;
		counter = 0;
	}
};


struct BinarySequencerWidget : ModuleWidget {
	BinarySequencerWidget(BinarySequencer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BinarySequencer.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// CV knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, BinarySequencer::DIV01_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, BinarySequencer::DIV02_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, BinarySequencer::DIV04_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, BinarySequencer::DIV08_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, BinarySequencer::DIV16_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, BinarySequencer::DIV32_PARAM));
		
		// other knobs
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, BinarySequencer::CLOCKRATE_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, BinarySequencer::LAG_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, BinarySequencer::LAGSHAPE_PARAM));
	
		// scale switch
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, BinarySequencer::SCALE_PARAM));
		
		// clock, run and reset inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, BinarySequencer::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, BinarySequencer::RUN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, BinarySequencer::RESET_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, BinarySequencer::CLOCK_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, BinarySequencer::TRIGGER_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, BinarySequencer::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, BinarySequencer::INV_OUTPUT));

		// S & H input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, BinarySequencer::SH_INPUT));

		// clock rate light
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_HALF_ROWS6(STD_ROW1)), module, BinarySequencer::CLOCK_LIGHT));
	}
	
	struct InitMenuItem : MenuItem {
		BinarySequencerWidget *widget;
		bool triggerInit = true;
		bool cvInit = true;
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize division mix";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			widget->getParam(BinarySequencer::DIV01_PARAM)->reset();
			widget->getParam(BinarySequencer::DIV02_PARAM)->reset();
			widget->getParam(BinarySequencer::DIV04_PARAM)->reset();
			widget->getParam(BinarySequencer::DIV08_PARAM)->reset();
			widget->getParam(BinarySequencer::DIV16_PARAM)->reset();
			widget->getParam(BinarySequencer::DIV32_PARAM)->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		BinarySequencerWidget *widget;
		bool cvRand = true;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize division mix";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			widget->getParam(BinarySequencer::DIV01_PARAM)->randomize();
			widget->getParam(BinarySequencer::DIV02_PARAM)->randomize();
			widget->getParam(BinarySequencer::DIV04_PARAM)->randomize();
			widget->getParam(BinarySequencer::DIV08_PARAM)->randomize();
			widget->getParam(BinarySequencer::DIV16_PARAM)->randomize();
			widget->getParam(BinarySequencer::DIV32_PARAM)->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
	
	void appendContextMenu(Menu *menu) override {
		BinarySequencer *module = dynamic_cast<BinarySequencer*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// pretty heading
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Binary Sequencer";
		menu->addChild(settingsLabel);		

		// CV only init
		InitMenuItem *initCVMenuItem = createMenuItem<InitMenuItem>("Initialize Division Mix Only");
		initCVMenuItem->widget = this;
		menu->addChild(initCVMenuItem);

		// CV only random
		RandMenuItem *randCVMenuItem = createMenuItem<RandMenuItem>("Randomize Division Mix Only");
		randCVMenuItem->widget = this;
		menu->addChild(randCVMenuItem);
	}
};

Model *modelBinarySequencer = createModel<BinarySequencer, BinarySequencerWidget>("BinarySequencer");
