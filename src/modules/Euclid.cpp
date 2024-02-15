//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Euclid Module
//	An Euclidean Trigger sequencer
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/EuclideanAlgorithm.hpp"
#include "../inc/EuclidExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Euclid
#define PANEL_FILE "Euclid.svg"

struct Euclid : Module {

	enum ParamIds {
		LENGTH_PARAM,
		HITS_PARAM,
		SHIFT_PARAM,
		LENGTH_CV_PARAM,
		HITS_CV_PARAM,
		SHIFT_CV_PARAM,
		SHIFT_R_PARAM,
		SHIFT_L_PARAM,
		SHIFT_MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		RUN_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		LENGTH_INPUT,
		HITS_INPUT,
		SHIFT_INPUT,
		SHIFT_R_INPUT,
		SHIFT_L_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		HTRIG_OUTPUT,
		HGATE_OUTPUT,
		RTRIG_OUTPUT,
		RGATE_OUTPUT,
		END_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(STEP_LIGHTS, EUCLID_SEQ_MAX_LEN * 3),
		HTRIG_LIGHT,
		HGATE_LIGHT,
		RTRIG_LIGHT,
		RGATE_LIGHT,
		END_LIGHT,
		SHIFT_R_PARAM_LIGHT,
		SHIFT_L_PARAM_LIGHT,
		NUM_LIGHTS
	};
	
	enum shiftSources {
		CV_SHIFT_MODE,
		MANUAL_SHIFT_MODE,
		AUTO_R_SHIFT_MODE,
		AUTO_L_SHIFT_MODE
	};
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	GateProcessor gateFwd;
	GateProcessor gateRev;
	
	dsp::PulseGenerator pgTrig;
	dsp::PulseGenerator pgEOC;
	dsp::PulseGenerator pgClock;

	EuclideanAlgorithm euclid;
	
	short stepnum = 0;
	bool quantizeChanges = true;
	
	int displayLength = 8;
	int displayShift = 0;
	int displayHits = 4;
	
	int startUpCounter = 0;
	int count = -1;
	int length = 8;
	int shift = 0;
	int hits = 4;
	
	float lengthCV = 0.0f;
	float shiftCV = 0.0f;
	float hitsCV = 0.0f;
	
	int shiftSource = 1;
	float fMaxSeqLen = (float)(EUCLID_SEQ_MAX_LEN)/10.0f;
	bool running = false;	
	
	int moduleVersion;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	EuclidExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	int expanderCounterBeats = -1;
	int expanderCounterRests = -1;
	int expanderCounter = -1;
	
	Euclid() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// length param
		configParam(LENGTH_PARAM, 1.0f, (float)(EUCLID_SEQ_MAX_LEN), 8.0f, "Length", " Steps");
		configParam(HITS_PARAM, 0.0f, 1.0f, 0.5f, "Number of hits", " % of length", 0.0f, 100.0f, 0.0f);
		configParam(SHIFT_PARAM, 0.0f, 1.0f, 0.0f, "Shift", " % of length", 0.0f, 100.0f, 0.0f);

		configParam(LENGTH_CV_PARAM,  -1.0f, 1.0f, 0.0f, "Length CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(HITS_CV_PARAM,  -1.0f, 1.0f, 0.0f, "Number of hits CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(SHIFT_CV_PARAM,  -1.0, 1.0, 0.0f, "Shift CV amount", " %", 0.0f, 100.0f, 0.0f);
		
		configButton(SHIFT_R_PARAM, "Step left");
		configButton(SHIFT_L_PARAM, "Step right");
		
		configSwitch(SHIFT_MODE_PARAM, 0.0f, 3.0f, 0.0f, "Shift source", {"CV", "Step buttons", "Left at end of cylce", "Right at end of Cycle"});
		
		configInput(RUN_INPUT, "Run");
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(LENGTH_INPUT, "Length CV");
		configInput(HITS_INPUT, "Number of hits CV");
		configInput(SHIFT_INPUT, "Shift CV");
		configInput(SHIFT_R_INPUT, "Step left trigger");
		configInput(SHIFT_L_INPUT, "Step right trigger");
		
		configOutput(HTRIG_OUTPUT, "Hit trigger");
		configOutput(HGATE_OUTPUT, "Hit gate");
		configOutput(RTRIG_OUTPUT, "Rest trigger");
		configOutput(RGATE_OUTPUT, "Rest gate");
		configOutput(END_OUTPUT, "End of cycle");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		//running = true;
		quantizeChanges = true;
		moduleVersion = 2;
		
		startUpCounter = 1;
		
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(moduleVersion));

		json_object_set_new(root, "currentStep", json_integer(count));
		json_object_set_new(root, "shiftPosition", json_integer(shift));
		json_object_set_new(root, "clockState", json_boolean(gateClock.high()));
		json_object_set_new(root, "runState", json_boolean(gateRun.high()));
		json_object_set_new(root, "quantizeChanges", json_boolean(quantizeChanges));
		json_object_set_new(root, "lengthCV", json_real(lengthCV));
		json_object_set_new(root, "shiftCV", json_real(shiftCV));
		json_object_set_new(root, "hitsCV", json_real(hitsCV));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"
				
		return root;
	}

	void dataFromJson(json_t *root) override {
		json_t *ver = json_object_get(root, "moduleVersion");
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *shiftPosition = json_object_get(root, "shiftPosition");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "runState");
		json_t *qc = json_object_get(root, "quantizeChanges");
		
		json_t *lcv = json_object_get(root, "lengthCV");
		json_t *scv = json_object_get(root, "shiftCV");
		json_t *hcv = json_object_get(root, "hitsCV");
		
		if (ver)
			moduleVersion = json_integer_value(ver);
		
		if (currentStep)
			count = json_integer_value(currentStep);

		if (shiftPosition)
			shift = json_integer_value(shiftPosition);
		
		if (clk)
			gateClock.preset(json_boolean_value(clk));

		if (run)
			gateRun.preset(json_boolean_value(run));
		
		running = gateRun.high();
		
		// upgrade to module version 2
		if (moduleVersion< 2) {
			quantizeChanges = false;
			moduleVersion = 2;
		}

		if (qc)
			quantizeChanges = json_boolean_value(qc);

		lengthCV = 0.0f;
		if(lcv)
			lengthCV = json_number_value(lcv);
		
		shiftCV = 0.0f;
		if(scv)
			shiftCV = json_number_value(scv);
		
		hitsCV = 0.0f;
		if(hcv)
			hitsCV = json_number_value(hcv); 

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		startUpCounter = 1;		
	}

	void onReset() override {
		count = -1;
		displayLength = length = 8;
		displayShift = shift = 0;
		displayHits = hits = 4;
		shiftSource = 1;
		stepnum = 0;
		
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
		gateFwd.reset();
		gateRev.reset();
		
		expanderCounterBeats = -1;
		expanderCounterRests = -1;
		expanderCounter = -1;
		
		pgClock.reset();
		pgTrig.reset();
		pgEOC.reset();
		euclid.reset();
		
		lengthCV = 0.0f;
		shiftCV = 0.0f;
		hitsCV = 0.0f;
		
		startUpCounter = 1;
	}

	void process(const ProcessArgs &args) override {

		bool processControls = (stepnum == 0);
	
		// reset input
		float f = inputs[RESET_INPUT].getVoltage();
		gateReset.set(f);
		
		// run input
		f = inputs[RUN_INPUT].getNormalVoltage(10.0f);
		gateRun.set(f);

		// clock
		f = inputs[CLOCK_INPUT].getVoltage(); 
		gateClock.set(f);

		// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
		bool clockEdge = gateClock.leadingEdge();
		if (clockEdge)
			pgClock.trigger(1e-4f);
		else if (pgClock.process(args.sampleTime)) {
			// if within cooey of the clock edge, run or reset is treated as a clock edge.
			clockEdge = (gateRun.leadingEdge() || gateReset.leadingEdge());
		}

		if (clockEdge && quantizeChanges) {
			lengthCV = fMaxSeqLen * params[LENGTH_CV_PARAM].getValue() * inputs[LENGTH_INPUT].getVoltage();
			hitsCV = params[HITS_CV_PARAM].getValue() * inputs[HITS_INPUT].getVoltage() * 0.1f; 
			shiftCV = params[SHIFT_CV_PARAM].getValue() * inputs[SHIFT_INPUT].getVoltage() * 0.1f;		
		}

		if (processControls || startUpCounter > 0) {
			
			if (!quantizeChanges) {
				lengthCV = fMaxSeqLen * params[LENGTH_CV_PARAM].getValue() * inputs[LENGTH_INPUT].getVoltage();
				hitsCV = params[HITS_CV_PARAM].getValue() * inputs[HITS_INPUT].getVoltage() * 0.1f;
				shiftCV = params[SHIFT_CV_PARAM].getValue() * inputs[SHIFT_INPUT].getVoltage() * 0.1f;
			}

			// length
			length = clamp((int)roundf(params[LENGTH_PARAM].getValue() + lengthCV), 1, EUCLID_SEQ_MAX_LEN);
			
			// hits
			f = (float)length * (params[HITS_PARAM].getValue() + hitsCV);
			hits = clamp((int)f, 0, length);
		
			//shift source
			shiftSource = (int)(params[SHIFT_MODE_PARAM].getValue());
			
			// handle the CV and manual step shift modes
			switch (shiftSource) {
				case CV_SHIFT_MODE:
					f = (float)length * (params[SHIFT_PARAM].getValue() + shiftCV);
					shift = clamp((int)f, 0, length);
					
					break;
				case MANUAL_SHIFT_MODE:
					f = max(inputs[SHIFT_R_INPUT].getVoltage(), params[SHIFT_R_PARAM].getValue() * 2.0f);
					gateFwd.set(f);
					
					// shift the pattern 1 step right - watch the logic here, it actually goes the other way
					if (gateFwd.leadingEdge()) {
						if (--shift < 0)
							shift = length -1;
					}
					
					f = max(inputs[SHIFT_L_INPUT].getVoltage(), params[SHIFT_L_PARAM].getValue() * 2.0f);
					gateRev.set(f);
					
					if (gateRev.leadingEdge()) {
						// shift the pattern 1 step left - watch the logic here, it actually goes the other way
						if (++shift >= length)
							shift = 0;
					}
					
				break;
			}
			
			displayLength = length;
			displayShift = shift;
			displayHits = hits;					
		}
		
		if (gateReset.leadingEdge()) {
			// reset the count
			count = -1;
			
			expanderCounterBeats = -1;
			expanderCounterRests = -1;
			expanderCounter = -1;
		}

		bool gate = false;
		bool igate = false;
		bool active = false;
		bool current = false;
		bool last = false;
	
		if (gateRun.low())
			running = false;
		
		// advance count on positive clock edge or the run edge if it is close to the clock edge
		bool trig = false;
		if (clockEdge && gateRun.high()) {

			// flag that we are now actually running
			running = true;
			
			count++;
			
			if (count >= length) {
				pgEOC.trigger(1e-3f);
				count = 0;
			
				// handle the auto shift function on the leading edge of the next step after the last step
				switch (shiftSource) {
					case AUTO_R_SHIFT_MODE:
						// shift the pattern 1 step right - watch the logic here, it actually goes the other way
						if (--shift < 0)
							shift = length - 1;
					
						break;
					case AUTO_L_SHIFT_MODE:
						// shift the pattern 1 step left - watch the logic here, it actually goes the other way
						if (++shift >= length)
							shift = 0;
				}
			}
		
			// trigger on every leading clock edge -  we'll sort out which trigger output to use later
			trig = true;
			pgTrig.trigger(1e-3f);
		}
		
		// update the euclid parameters - flag that a change has occurred so we can update the display
		processControls = (euclid.set(hits, length, -shift) || startUpCounter > 0 || clockEdge);
		
		if (running || processControls) {
			for (int i = 0; i < EUCLID_SEQ_MAX_LEN; i ++) {
				active = euclid.pattern(i);
				if (running) {
					gate |= (active && i == count);
					igate = !gate;
				}
				current = (i == count);
				last = (i == length -1);
				if(processControls) {
					lights[STEP_LIGHTS + (i * 3)].setBrightness(boolToLight(current)); // Red - current step
					lights[STEP_LIGHTS + (i * 3) + 1].setBrightness(boolToLight(active)); // Green - active steps (hits)
					lights[STEP_LIGHTS + (i * 3) + 2].setBrightness(boolToLight(last)); // Blue - length
				}
			}
		}

		lights[HGATE_LIGHT].setBrightness(boolToLight(gate));
		outputs[HGATE_OUTPUT].setVoltage(boolToGate(gate));
		lights[RGATE_LIGHT].setBrightness(boolToLight(igate));
		outputs[RGATE_OUTPUT].setVoltage(boolToGate(igate));
		
		if (!trig)
			trig = pgTrig.process(args.sampleTime);
		
		lights[HTRIG_LIGHT].setSmoothBrightness(boolToLight(gate && trig), args.sampleTime);
		outputs[HTRIG_OUTPUT].setVoltage(boolToGate(gate && trig));
		lights[RTRIG_LIGHT].setSmoothBrightness(boolToLight(igate && trig), args.sampleTime);
		outputs[RTRIG_OUTPUT].setVoltage(boolToGate(igate && trig));
		
		bool end = pgEOC.remaining > 0.0f;
		outputs[END_OUTPUT].setVoltage(boolToGate(end));
		lights[END_LIGHT].setSmoothBrightness(boolToLight(end), args.sampleTime);
		pgEOC.process(args.sampleTime);
			
		if(++stepnum > 8)
			stepnum = 0;
		
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				EuclidExpanderMessage *messageToExpander = (EuclidExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				if (gateClock.leadingEdge()) {
					int maxsteps = std::min(EUCLID_EXP_NUM_STEPS, length);
					
					if(++expanderCounter >= maxsteps)
						expanderCounter = 0;
					
					if (gate) {
						if (++expanderCounterBeats >= maxsteps)
							expanderCounterBeats = 0;
					}
					else {
						if (++expanderCounterRests >= maxsteps)
							expanderCounterRests = 0;
					}
				}

				messageToExpander->set(gate, igate, clockEdge, gateClock.high(), trig, running, expanderCounterBeats, expanderCounterRests, expanderCounter, 1, true);				

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}
		
		if (startUpCounter > 0)
			startUpCounter--;
	}
};

struct EuclidWidget : ModuleWidget {

	std::string panelName;
	CountModulaLEDDisplayMini2 *lengthDisplay;
	CountModulaLEDDisplayMini2 *hitsDisplay;
	CountModulaLEDDisplayMini2 *shiftDisplay;
	
	EuclidWidget(Euclid *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// run, reset and clock input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Euclid::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, Euclid::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, Euclid::RUN_INPUT));
				
		// length, hits and shift params
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, Euclid::LENGTH_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, Euclid::HITS_PARAM));
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, Euclid::SHIFT_PARAM));
		
		// length, hits and shift inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, Euclid::LENGTH_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, Euclid::HITS_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Euclid::SHIFT_INPUT));

		// length, hits and shift CV params
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Euclid::LENGTH_CV_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Euclid::HITS_CV_PARAM));
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Euclid::SHIFT_CV_PARAM));
		
		// shift up/dn and mode
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Euclid::SHIFT_R_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Euclid::SHIFT_L_INPUT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Euclid::SHIFT_R_PARAM, Euclid::SHIFT_R_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Euclid::SHIFT_L_PARAM, Euclid::SHIFT_L_PARAM_LIGHT));
		addParam(createParamCentered<RotarySwitch<OperatingAngle145<WhiteKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_HALF_ROWS6(STD_ROW4)), module, Euclid::SHIFT_MODE_PARAM));
		
		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS6[STD_ROW1] + 20;
		for (int s = 0; s < EUCLID_SEQ_MAX_LEN; s++) {
			if (x > 7) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] - 5 + (10 * x++), startPos + (10 * y)), module, Euclid::STEP_LIGHTS + (s* 3)));
		}
		
		// output lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 15, STD_ROWS6[STD_ROW5] - 19), module, Euclid::HTRIG_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL9] + 15, STD_ROWS6[STD_ROW5] - 19), module, Euclid::HGATE_LIGHT));

		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 15, STD_ROWS6[STD_ROW6] - 19), module, Euclid::RTRIG_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL9] + 15, STD_ROWS6[STD_ROW6] - 19), module, Euclid::RGATE_LIGHT));
			
		// gate/trig output jack
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW5]), module, Euclid::HTRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS6[STD_ROW5]), module, Euclid::HGATE_OUTPUT));

		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW6]), module, Euclid::RTRIG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS6[STD_ROW6]), module, Euclid::RGATE_OUTPUT));
		
		// EOC output and light
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL8], STD_ROWS6[STD_ROW4]), module, Euclid::END_OUTPUT));
		addChild(createLightCentered<SmallLight<BlueLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL8] + 15, STD_ROWS6[STD_ROW4] - 19), module, Euclid::END_LIGHT));
		
		// parameter displays
		lengthDisplay = new CountModulaLEDDisplayMini2();
		lengthDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]));
		lengthDisplay->text = "08";
		addChild(lengthDisplay);

		hitsDisplay = new CountModulaLEDDisplayMini2();
		hitsDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL8], STD_ROWS6[STD_ROW1]));
		hitsDisplay->text = "04";
		addChild(hitsDisplay);

		shiftDisplay = new CountModulaLEDDisplayMini2();
		shiftDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS6[STD_ROW1]));
		shiftDisplay->text = "00";
		addChild(shiftDisplay);
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	struct InitMenuItem : MenuItem {
		EuclidWidget *widget;
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "initialize pattern";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			// step controls
			widget->getParam(Euclid::LENGTH_PARAM)->getParamQuantity()->reset();
			widget->getParam(Euclid::HITS_PARAM)->getParamQuantity()->reset();
			widget->getParam(Euclid::SHIFT_PARAM)->getParamQuantity()->reset();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);
		}
	};	
	
	struct RandMenuItem : MenuItem {
		EuclidWidget *widget;

		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = "randomize pattern";
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			widget->getParam(Euclid::LENGTH_PARAM)->getParamQuantity()->randomize();
			widget->getParam(Euclid::HITS_PARAM)->getParamQuantity()->randomize();
			widget->getParam(Euclid::SHIFT_PARAM)->getParamQuantity()->randomize();

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
	
	struct QuantizeMenuItem : MenuItem {
		Euclid* module;

		void onAction(const event::Action &e) override {
			module->quantizeChanges ^= true;
		}
	};		
	
	// expander addition menu item
	#include "../inc/AddExpanderMenuItem.hpp"	
	
	void appendContextMenu(Menu *menu) override {
		Euclid *module = dynamic_cast<Euclid*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"	
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));
		
		// quantize option
		QuantizeMenuItem *quantizeMenuItem = createMenuItem<QuantizeMenuItem>("Quantize", CHECKMARK(module->quantizeChanges));
		quantizeMenuItem->module = module;
		menu->addChild(quantizeMenuItem);
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Pattern"));		
		// pattern only init
		InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>("Initialize Pattern");
		initMenuItem->widget = this;
		menu->addChild(initMenuItem);

		// pattern only random
		RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>("Randomize Pattern");
		randMenuItem->widget = this;
		menu->addChild(randMenuItem);
		
		//add the expander menu item		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Expansion"));
		AddExpanderMenuItem *expMenuItem = createMenuItem<AddExpanderMenuItem>("Add CV expander");
		expMenuItem->module = module;
		expMenuItem->model = modelEuclidExpanderCV;
		expMenuItem->position = box.pos;
		expMenuItem->expanderName = "CV";
		menu->addChild(expMenuItem);			
	}
	
	void step() override {
		if (module) {
			Euclid *m = (Euclid *)module;

			lengthDisplay->text = rack::string::f( "%02d", m->displayLength);
			hitsDisplay->text =  rack::string::f( "%02d", m->displayHits);
			shiftDisplay->text =  rack::string::f( "%02d", m->displayShift);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};

Model *modelEuclid = createModel<Euclid, EuclidWidget>("Euclid");
