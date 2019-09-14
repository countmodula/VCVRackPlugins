//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Binary Sequencer Module
//	VCV Rack version of now extinct Blacet Binary Zone Frac Module.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockOscillator.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BurstGenerator
#define PANEL_FILE "BurstGenerator.svg"

struct BurstGenerator : Module {
	enum ParamIds {
		PULSES_PARAM,
		RATE_PARAM,
		RATECV_PARAM,
		RANGE_PARAM,
		RETRIGGER_PARAM,
		PULSESCV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RATECV_INPUT,
		TRIGGER_INPUT,
		PULSESCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PULSES_OUTPUT,
		START_OUTPUT,
		DURATION_OUTPUT,
		END_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		NUM_LIGHTS
	};

	int counter = -1;
	bool bursting = false;
	bool prevBursting = false;
	bool startBurst = false;
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	bool seqBurst = false;
	int seqCount = 0;
#endif

	bool state = false;
	float clockFreq = 1.0f;
	
	ClockOscillator clock;
	GateProcessor gpClock;
	GateProcessor gpTrig;
	dsp::PulseGenerator pgStart;
	dsp::PulseGenerator pgEnd;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif	
	
	BurstGenerator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(RATECV_PARAM, -1.0f, 1.0f, 0.0f, "Rate CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(RATE_PARAM, 0.0f, 5.0f, 0.0f, "Burst rate");
		configParam(RANGE_PARAM, 0.0f, 1.0f, 0.0f, "Rate range");
		configParam(RETRIGGER_PARAM, 0.0f, 1.0f, 0.0f, "Retrigger On/Off");
		configParam(PULSESCV_PARAM, -1.6f, 1.6f, 0.0f, "Number of pulses CV amount", " %", 0.0f, 62.5f, 0.0f);
		configParam(PULSES_PARAM, 1.0f, 16.0f, 1.0f, "Number of pulses");
		configParam(MANUAL_PARAM, 0.0f, 1.0f, 0.0f, "Manual trigger");
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
#endif			

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		gpClock.reset();
		gpTrig.reset();
		pgStart.reset();
		pgEnd.reset();
		clock.reset();
		bursting = false;
		seqBurst = false;
		seqCount = 0;
		counter = -1;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.1"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
	
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {

		// grab the current burst count taking CV into account ans ensuring we don't go below 1
		int pulseCV = clamp(inputs[PULSESCV_INPUT].getVoltage(), -10.0f, 10.0f) * params[PULSESCV_PARAM].getValue();
		int pulses = (int)fmaxf(params[PULSES_PARAM].getValue() + pulseCV, 1.0f); 

		// determine clock rate
		float rateCV = clamp(inputs[RATECV_INPUT].getVoltage(), -10.0f, 10.0f) * params[RATECV_PARAM].getValue();
		float rate = params[RATE_PARAM].getValue() + rateCV;
		float range = params[RANGE_PARAM].getValue();
		if (range > 0.0f) {
			rate = 4.0f + (rate * 2.0f);
		}
		
		// now set it
		clock.setPitch(rate);
		
		// set the trigger input value
		gpTrig.set(fmaxf(inputs[TRIGGER_INPUT].getVoltage(), params[MANUAL_PARAM].getValue() * 10.0f));
		bool retrigAllowed = params[RETRIGGER_PARAM].getValue() > 0.5f;
		
		// leading edge of trigger input fires off the burst if we can
		if (gpTrig.leadingEdge()) {
			if (!bursting || (bursting && retrigAllowed)) {
				gpClock.reset();
				clock.reset();
		
				// set the burst to go off
				startBurst = true;
				counter = -1;
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
				seqBurst = true;
				seqCount = 0;
#endif
			}
		}
		
		// tick the internal clock over here as we could have reset the clock above
		clock.step(args.sampleTime);

		// get the clock value we want to use (internal or external)
		float internalClock = 5.0f * clock.sqr();
		float clockState = inputs[CLOCK_INPUT].getNormalVoltage(internalClock);
		gpClock.set(clockState);

		// process the burst logic based on the results of the above
		if (gpClock.leadingEdge()) {
#ifdef SEQUENCER_EXP_MAX_CHANNELS				
			// keep a separate count for the sequencer expanders
			if (seqBurst) {
				seqCount++;
				if (seqCount > pulses) {	
					seqCount = 0;
					seqBurst = false;
				}
			}
#endif

			if (startBurst || bursting) {
				if (++counter >= pulses) {
					counter = -1;
					bursting = false;
				}
				else {
					bursting = true;
				}
				
				startBurst = false;
			}
		}
		
		// end the duration after the last pulse, not at the next clock cycle
		if (gpClock.trailingEdge() && counter + 1 >= pulses)
			bursting = false;
		
		// set the duration start trigger if we've changed from not bursting to bursting
		if (!prevBursting && bursting) {
			pgStart.trigger(1e-3f);
		}
		
		// set the duration end trigger if we've changed from bursting to not bursting
		if (prevBursting && !bursting) {
			pgEnd.trigger(1e-3f);
		}
			
		// finally set the outputs as required
		outputs[PULSES_OUTPUT].setVoltage(boolToGate(bursting && gpClock.high()));
		outputs[DURATION_OUTPUT].setVoltage(boolToGate(bursting));
		outputs[START_OUTPUT].setVoltage(boolToGate(pgStart.process(args.sampleTime)));
		outputs[END_OUTPUT].setVoltage(boolToGate(pgEnd.process(args.sampleTime)));
		
		// blink the clock light according to the clock rate
		lights[CLOCK_LIGHT].setSmoothBrightness(gpClock.light(), args.sampleTime);
		
		// save bursting state for next step
		prevBursting = bursting;
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set any potential expander module's channel number
				messageToExpander->setAllChannels(0);
		
				// add the channel counters and gates
				int c = seqBurst ? counter + 1 : 0;
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS ; i++) {
					messageToExpander->counters[i] = c;
					messageToExpander->clockStates[i] =	gpClock.high();
					messageToExpander->runningStates[i] = true; // always running - the counter takes care of the not running states 
				}
			
				// finally, let all potential expanders know where we came from
				messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_BURSTGENERATOR;
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
#endif		
	}
};

struct BurstGeneratorWidget : ModuleWidget {
	BurstGeneratorWidget(BurstGenerator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BurstGenerator.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));	
		
		// controls
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, BurstGenerator::RATECV_PARAM));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, BurstGenerator::RATE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, BurstGenerator::RANGE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, BurstGenerator::RETRIGGER_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, BurstGenerator::PULSESCV_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitchGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, BurstGenerator::PULSES_PARAM));
		
		// manual trigger button
		addParam(createParamCentered<CountModulaPBSwitchBigMomentary>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, BurstGenerator::MANUAL_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, BurstGenerator::RATECV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, BurstGenerator::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, BurstGenerator::PULSESCV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, BurstGenerator::TRIGGER_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, BurstGenerator::PULSES_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, BurstGenerator::START_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, BurstGenerator::DURATION_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, BurstGenerator::END_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, BurstGenerator::CLOCK_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		BurstGenerator *module = dynamic_cast<BurstGenerator*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelBurstGenerator = createModel<BurstGenerator, BurstGeneratorWidget>("BurstGenerator");
