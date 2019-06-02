//----------------------------------------------------------------------------
//	Count Modula - Binary Sequencer Module
//	VCV Rack version of now extinct Blacet Binary Zone Frac Module.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockOscillator.hpp"
#include "../inc/GateProcessor.hpp"

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
	
	bool state = false;
	float clockFreq = 1.0f;
	
	ClockOscillator clock;
	GateProcessor gpClock;
	GateProcessor gpTrig;
	PulseGenerator pgStart;
	PulseGenerator pgEnd;
	
	BurstGenerator() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	void onReset() override {
		gpClock.reset();
		gpTrig.reset();
		pgStart.reset();
		pgEnd.reset();
		clock.reset();
		bursting = false;
		counter = -1;
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void BurstGenerator::step() {

	// grab the current burst count taking CV into account ans ensuring we don't go below 1
	int pulseCV = clamp(inputs[PULSESCV_INPUT].value, -10.0f, 10.0f) * params[PULSESCV_PARAM].value;
	int pulses = (int)fmaxf(params[PULSES_PARAM].value + pulseCV, 1.0f); 

	// determine clock rate
	float rateCV = clamp(inputs[RATECV_INPUT].value, -10.0f, 10.0f) * params[RATECV_PARAM].value;
	float rate = params[RATE_PARAM].value + rateCV;
	float range = params[RANGE_PARAM].value;
	if (range > 0.0f) {
		rate = 4.0f + (rate * 2.0f);
	}
	
	// now set it
	clock.setPitch(rate);
	
	// set the trigger input value
	gpTrig.set(fmaxf(inputs[TRIGGER_INPUT].value, params[MANUAL_PARAM].value * 10.0f));
	bool retrigAllowed = params[RETRIGGER_PARAM].value > 0.5f;
	
	// leading edge of trigger input fires off the burst if we can
	if (gpTrig.leadingEdge()) {
		if (!bursting || (bursting && retrigAllowed)) {
			gpClock.reset();
			clock.reset();
	
			// set the burst to go off
			startBurst = true;
			counter = -1;
		}
	}
	
	// tick the internal clock over here as we could have reset the clock above
	clock.step(engineGetSampleTime());

	// get the clock value we want to use (internal or external)
	float internalClock = 5.0f * clock.sqr();
	float clockState = inputs[CLOCK_INPUT].normalize(internalClock);
	gpClock.set(clockState);

	// process the burst logic based on the results of the above
	if (gpClock.leadingEdge()) {
		if (startBurst || bursting) {
			if (++counter >= pulses) {
				counter = -1;
				bursting = false;
			}
			else
				bursting = true;

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
	outputs[PULSES_OUTPUT].value = boolToGate(bursting && gpClock.high());
	outputs[DURATION_OUTPUT].value = boolToGate(bursting);
	outputs[START_OUTPUT].value = boolToGate(pgStart.process(engineGetSampleTime()));
	outputs[END_OUTPUT].value = boolToGate(pgEnd.process(engineGetSampleTime()));
	
	// blink the clock light according to the clock rate
	lights[CLOCK_LIGHT].setBrightnessSmooth(gpClock.light());
	
	// save bursting state for next step
	prevBursting = bursting;
}


struct BurstGeneratorWidget : ModuleWidget {
	BurstGeneratorWidget(BurstGenerator *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/BurstGenerator.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));	
		
		// controls
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, BurstGenerator::RATECV_PARAM, -1.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, BurstGenerator::RATE_PARAM, 0.0f, 5.0f, 0.0f));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, BurstGenerator::RANGE_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, BurstGenerator::RETRIGGER_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, BurstGenerator::PULSESCV_PARAM, -1.6f, 1.6f, 0.0f));
		addParam(createParamCentered<CountModulaRotarySwitchGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, BurstGenerator::PULSES_PARAM, 1.0f, 16.0f, 1.0f));
		
		// manual trigger button
		addParam(createParamCentered<CountModulaPBSwitchBigMomentary>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, BurstGenerator::MANUAL_PARAM, 0.0f, 1.0f, 0.0f));
		
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
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelBurstGenerator = Model::create<BurstGenerator, BurstGeneratorWidget>("Count Modula", "BurstGenerator", "Burst Generator", SEQUENCER_TAG, CLOCK_TAG);
