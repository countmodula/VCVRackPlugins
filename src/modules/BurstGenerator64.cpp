//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Burst Generator 64
//	Copyright (C) 2023  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockOscillator.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BurstGenerator64
#define PANEL_FILE "BurstGenerator64.svg"

#define PROCESSCOUNT 32
struct BurstGenerator64 : Module {
	enum ParamIds {
		PULSES_PARAM,
		RATE_PARAM,
		RATECV_PARAM,
		RANGE_PARAM,
		RETRIGGER_PARAM,
		PULSESCV_PARAM,
		MANUAL_PARAM,
		PULSEPROB_PARAM,
		PULSEPROBCV_PARAM,
		JITTER_PARAM,
		CLOCKPROB_PARAM,
		CLOCKPROBCV_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RATECV_INPUT,
		TRIGGER_INPUT,
		PULSESCV_INPUT,
		PULSEPROBCV_INPUT,
		JITTER_INPUT,
		CLOCKPROBCV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PULSES_OUTPUT,
		START_OUTPUT,
		DURATION_OUTPUT,
		END_OUTPUT,
		CLOCK_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		MANUAL_PARAM_LIGHT,
		PULSES_LIGHT,
		START_LIGHT,
		DURATION_LIGHT,
		END_LIGHT,
		NUM_LIGHTS
	};

	int counter = -1;
	bool bursting = false;
	bool prevBursting = false;
	bool startBurst = false;
	bool prob = true;
	bool clProb = true;
	
	bool state = false;
	float clockFreq = 1.0f;
	int processCount = PROCESSCOUNT;
	
	float pulseParam = 0.0f;
	float rateParam = 0.0f;
	float rateCVParam = 0.0f;
	float rangeParam = 0.0f;
	float retriggerParam = 0.0f;
	float pulsesCVParam = 0.0f;
	float pulseProbParam = 0.0f;
	float pulseProbCVParam = 0.0f;
	float jitterParam = 0.0f;
	float clockProbParam = 0.0f;
	float clockProbCVParam = 0.0f;
	
	ClockOscillator clock;
	GateProcessor gpClock;
	GateProcessor gpTrig;
	dsp::PulseGenerator pgStart;
	dsp::PulseGenerator pgEnd;

	float jitter = 0.0f;
	bool internalClock = false;
	
	bool bypassProbOnClockOutput = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	BurstGenerator64() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(RATECV_PARAM, -1.0f, 1.0f, 0.0f, "Rate CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(RATE_PARAM, 0.0f, 5.0f, 0.0f, "Burst rate");
		configSwitch(RANGE_PARAM, 0.0f, 1.0f, 0.0f, "Rate range", {"Slow", "Fast"});
		configSwitch(RETRIGGER_PARAM, 0.0f, 1.0f, 0.0f, "Retrigger", {"Off", "On"});
		configParam(PULSESCV_PARAM, -6.4f, 6.4f, 0.0f, "Number of pulses CV amount", " %", 0.0f, 15.625f, 0.0f);
		configParam(PULSES_PARAM, 1.0f, 64.0f, 1.0f, "Number of pulses");
		configButton(MANUAL_PARAM, "Manual trigger");
		configParam(PULSEPROB_PARAM, 0.0f, 10.0f, 10.0f, "Pulse probability", " %", 0.0f, 10.0f, 0.0f);
		configParam(PULSEPROBCV_PARAM, -1.0f, 1.0f, 0.0f, "Pulse probability CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(JITTER_PARAM, 0.0f, 1.0f, 0.0f, "Jitter amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(CLOCKPROB_PARAM, 0.0f, 10.0f, 10.0f, "Clock probability", " %", 0.0f, 10.0f, 0.0f);
		configParam(CLOCKPROBCV_PARAM, -1.0f, 1.0f, 0.0f, "CLock probability CV amount", " %", 0.0f, 100.0f, 0.0f);
		
		configInput(CLOCK_INPUT, "External clock");
		inputInfos[CLOCK_INPUT]->description = "Disconnects the internal clock";
		configInput(RATECV_INPUT, "Internal rate CV");
		configInput(TRIGGER_INPUT, "Trigger");
		configInput(PULSESCV_INPUT, "Number of pulses CV");
		configInput(PULSEPROBCV_INPUT, "Pulse probability");
		configInput(JITTER_INPUT, "Clock jitter CV");
		configInput(CLOCKPROBCV_INPUT, "Clock probability CV");
		
		configOutput(PULSES_OUTPUT, "Pulse");
		configOutput(START_OUTPUT, "Start of burst");
		configOutput(DURATION_OUTPUT, "Burst duration");
		configOutput(END_OUTPUT, "End of burst");
		configOutput(CLOCK_OUTPUT, "Clock");

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
		counter = -1;
		processCount = PROCESSCOUNT;
		jitter = 0.0f;
		bypassProbOnClockOutput = false;
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		json_object_set_new(root, "bypassProbOnClockOutput", json_boolean(bypassProbOnClockOutput));

		// add the theme details
		#include "../themes/dataToJson.hpp"		
	
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"

		bypassProbOnClockOutput = false;
		json_t *cop = json_object_get(root, "bypassProbOnClockOutput");
		if (cop) {
			bypassProbOnClockOutput = json_boolean_value(cop);		
		}

		processCount = PROCESSCOUNT;
	}	
	
	void process(const ProcessArgs &args) override {

		// don't grab params at audio rate.
		if (++processCount > PROCESSCOUNT) {
			processCount = 0;
			
			pulseParam = params[PULSES_PARAM].getValue();
			rateParam = params[RATE_PARAM].getValue();
			rateCVParam = params[RATECV_PARAM].getValue();
			rangeParam = params[RANGE_PARAM].getValue();
			retriggerParam = params[RETRIGGER_PARAM].getValue();
			pulsesCVParam = params[PULSESCV_PARAM].getValue();
			pulseProbParam = params[PULSEPROB_PARAM].getValue();
			pulseProbCVParam = params[PULSEPROBCV_PARAM].getValue();
			jitterParam = params[JITTER_PARAM].getValue();
			clockProbParam = params[CLOCKPROB_PARAM].getValue();
			clockProbCVParam = params[CLOCKPROBCV_PARAM].getValue();
			
			// skip internal clock stuff if external clock is used.
			internalClock = !inputs[CLOCK_INPUT].isConnected();
		}
		
		// grab the current burst count taking CV into account and ensuring we don't go below 1
		int pulseCV = clamp(inputs[PULSESCV_INPUT].getVoltage(), -10.0f, 10.0f) * pulsesCVParam;
		int pulses = (int)fmaxf(pulseParam + pulseCV, 1.0f); 

		if (internalClock) {
			// calculate jitter amount for next cycle
			if (gpClock.trailingEdge()) {
				if (inputs[JITTER_INPUT].isConnected()) {
					jitter = clamp(inputs[JITTER_INPUT].getVoltage(), 0.0f, 10.0f)/10.0f * jitterParam * random::uniform() * 5.0f;
				}
				else {
					jitter = jitterParam * random::uniform() * 5.0f;
				}
			}
			
			
			// determine clock rate
			float rateCV = clamp(inputs[RATECV_INPUT].getVoltage(), -10.0f, 10.0f) * rateCVParam;
			float rate = rateParam + rateCV + jitter;
			
			if (rangeParam > 0.0f) {
				rate = 4.0f + (rate * 2.0f);
			}
			
			// now set it
			clock.setPitch(rate);
		}
		
		// set the trigger input value
		gpTrig.set(fmaxf(inputs[TRIGGER_INPUT].getVoltage(), params[MANUAL_PARAM].getValue() * 10.0f));
		
		// leading edge of trigger input fires off the burst if we can
		if (gpTrig.leadingEdge()) {
			
			if (!bursting || (bursting && retriggerParam > 0.5f)) {
				if (internalClock) {
					clock.reset();
				}
				
				gpClock.reset();
				
				// set the burst to go off
				startBurst = true;
				counter = -1;
			}
		}
		
		if (internalClock) {
			// tick the internal clock over here as we could have reset the clock above
			clock.step(args.sampleTime);
			gpClock.set(5.0f * clock.sqr());
		}
		else {
			gpClock.set(inputs[CLOCK_INPUT].getVoltage());
		}


		// process the burst logic based on the results of the above
		if (gpClock.leadingEdge()) {
			// check clock chance here
			float pCV = clamp (inputs[CLOCKPROBCV_INPUT].getVoltage() * clockProbCVParam, -10.0, 10.0);
			clProb = (random::uniform() <= clamp((clockProbParam + pCV) / 10.0f, 0.0f, 1.0f));
		
			// determine probability of pulse firing
			pCV = clamp (inputs[PULSEPROBCV_INPUT].getVoltage() * pulseProbCVParam, -10.0, 10.0);
			prob = clProb && (random::uniform() <= clamp((pulseProbParam + pCV) / 10.0f, 0.0f, 1.0f));
			
			// process burst count
			if (startBurst || bursting) {
				if (clProb && ++counter >= pulses) {
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
		bool startOut = false;
		if (!prevBursting && bursting) {
			pgStart.trigger(1e-3f);
		}
		else
			startOut = pgStart.process(args.sampleTime);
	
		// set the duration end trigger if we've changed from bursting to not bursting
		bool endOut = false;
		if (prevBursting && !bursting) {
			pgEnd.trigger(1e-3f);
		}
		else
			endOut = pgEnd.process(args.sampleTime);
		
		if (bursting && prob && gpClock.high()) {
			outputs[PULSES_OUTPUT].setVoltage(10.0f);
			lights[PULSES_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[PULSES_OUTPUT].setVoltage(0.0f);
			lights[PULSES_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		if (bursting) {
			outputs[DURATION_OUTPUT].setVoltage(10.0f);
			lights[DURATION_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[DURATION_OUTPUT].setVoltage(0.0f);
			lights[DURATION_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		if (startOut) {
			outputs[START_OUTPUT].setVoltage(10.0f);
			lights[START_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[START_OUTPUT].setVoltage(0.0f);
			lights[START_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		if (endOut) {
			outputs[END_OUTPUT].setVoltage(10.0f);
			lights[END_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[END_OUTPUT].setVoltage(0.0f);
			lights[END_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
		
		// blink the clock light according to the clock rate
		if ((bypassProbOnClockOutput || clProb) && gpClock.high()) {
			outputs[CLOCK_OUTPUT].setVoltage(10.0f);
			lights[CLOCK_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[CLOCK_OUTPUT].setVoltage(0.0f);
			lights[CLOCK_LIGHT].setBrightness(0.0f);
		}
		
		// save bursting state for next step
		prevBursting = bursting;
	}
};

struct BurstGenerator64Widget : ModuleWidget {

	std::string panelName;
	
	BurstGenerator64Widget(BurstGenerator64 *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		// clock section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, BurstGenerator64::RATECV_INPUT));
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, BurstGenerator64::RATECV_PARAM));
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, BurstGenerator64::RATE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, BurstGenerator64::RANGE_PARAM));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, BurstGenerator64::JITTER_INPUT));
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, BurstGenerator64::JITTER_PARAM));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]), module, BurstGenerator64::CLOCK_INPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW2]), module, BurstGenerator64::CLOCK_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7]-22, STD_ROWS6[STD_ROW2]-19), module, BurstGenerator64::CLOCK_LIGHT));
		
		// clock probability section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, BurstGenerator64::CLOCKPROBCV_INPUT));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, BurstGenerator64::CLOCKPROBCV_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, BurstGenerator64::CLOCKPROB_PARAM));
		
		// pulses section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, BurstGenerator64::PULSESCV_INPUT));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, BurstGenerator64::PULSESCV_PARAM));
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, BurstGenerator64::PULSES_PARAM));

		// pulse probability section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, BurstGenerator64::PULSEPROBCV_INPUT));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, BurstGenerator64::PULSEPROBCV_PARAM));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, BurstGenerator64::PULSEPROB_PARAM));

		// trigger section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, BurstGenerator64::TRIGGER_INPUT));
		addParam(createParamCentered<CountModulaLEDPushButtonBigMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, BurstGenerator64::MANUAL_PARAM, BurstGenerator64::MANUAL_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, BurstGenerator64::RETRIGGER_PARAM));
		
		// outputs + lights
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW3]), module, BurstGenerator64::START_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7]-22, STD_ROWS6[STD_ROW3]-19), module, BurstGenerator64::START_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW4]), module, BurstGenerator64::DURATION_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7]-22, STD_ROWS6[STD_ROW4]-19), module, BurstGenerator64::DURATION_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW5]), module, BurstGenerator64::END_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7]-22, STD_ROWS6[STD_ROW5]-19), module, BurstGenerator64::END_LIGHT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW6]), module, BurstGenerator64::PULSES_OUTPUT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7]-22, STD_ROWS6[STD_ROW6]-19), module, BurstGenerator64::PULSES_LIGHT));
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	// operational mode menu
	struct ClockOutputModeMenuItem : MenuItem {
		BurstGenerator64 *module;
	
		void onAction(const event::Action &e) override {
			module->bypassProbOnClockOutput ^= true;
		}
	};	
	
	
	void appendContextMenu(Menu *menu) override {
		BurstGenerator64 *module = dynamic_cast<BurstGenerator64*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));
		
		// add the output mode menu
		ClockOutputModeMenuItem *modeMenu = createMenuItem<ClockOutputModeMenuItem>("Bypass probability on clock output", CHECKMARK(module->bypassProbOnClockOutput));
		modeMenu->module = module;
		menu->addChild(modeMenu);
		
		
		
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelBurstGenerator64 = createModel<BurstGenerator64, BurstGenerator64Widget>("BurstGenerator64");
