//----------------------------------------------------------------------------
//	Count Modula - Binary Sequencer Module
//	VCV Rack version of now extinct Blacet Binary Zone Frac Module.
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/ClockOscillator.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/GateProcessor.hpp"

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
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	PulseGenerator  pgTrig;
	ClockOscillator clock;
	SlewLimiter slew;
	
	BinarySequencer() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	void onReset() override {
		gateClock.reset();
		gateReset.reset();
		pgTrig.reset();
		clock.reset();
		slew.reset();
		
		counter = 0;
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void BinarySequencer::step() {

	// generate the internal clock value
	clock.setPitch(params[CLOCKRATE_PARAM].value);
	clock.step(engineGetSampleTime());
	
	// handle the reset/run input
	gateReset.set(inputs[RESET_INPUT].normalize(10.0f));
	
	// grab the clock input value
	float internalClock = 5.0f * clock.sqr();
	float clockState = inputs[CLOCK_INPUT].normalize(internalClock);
	gateClock.set(clockState);

	
	if (gateReset.high()) {
		// is it a transition from low to high?
		if (gateClock.leadingEdge()) {
			
			// kick off the trigger
			pgTrig.trigger(1e-3f);
			
			if (++counter > 127)
				counter = 0;
		}
	}
	
	// determine current scale
	float scale = 0.0f;
	switch ((int)(params[SCALE_PARAM].value))
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

	// calculate the CV value
	float cv = 0.0f;
	if (gateReset.high())
	{
		if ((counter & 0x01) == 0x01) cv += params[DIV01_PARAM].value;
		if ((counter & 0x02) == 0x02) cv += params[DIV02_PARAM].value;
		if ((counter & 0x04) == 0x04) cv += params[DIV04_PARAM].value;
		if ((counter & 0x08) == 0x08) cv += params[DIV08_PARAM].value;
		if ((counter & 0x10) == 0x10) cv += params[DIV16_PARAM].value;
		if ((counter & 0x20) == 0x20) cv += params[DIV32_PARAM].value;
	}
	
	// scale the output to the currently selected value
	cv = clamp(cv * scale, -10.0f, 10.0f);
	
	// apply lag
	cv = slew.process(cv, params[LAGSHAPE_PARAM].value, params[LAG_PARAM].value, params[LAG_PARAM].value);
	
	// set the outputs
	outputs[CV_OUTPUT].value = 	cv;
	outputs[INV_OUTPUT].value = -cv;
	outputs[CLOCK_OUTPUT].value = gateClock.value();
	outputs[TRIGGER_OUTPUT].value = boolToGate(pgTrig.process(engineGetSampleTime()));
	
	// blink the light according to the clock
	lights[CLOCK_LIGHT].setBrightnessSmooth(gateClock.light());
}


struct BinarySequencerWidget : ModuleWidget {
	BinarySequencerWidget(BinarySequencer *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/BinarySequencer.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// CV knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, BinarySequencer::DIV01_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, BinarySequencer::DIV02_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, BinarySequencer::DIV04_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, BinarySequencer::DIV08_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, BinarySequencer::DIV16_PARAM, -10.0f, 10.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, BinarySequencer::DIV32_PARAM, -10.0f, 10.0f, 0.0f));
		
		// other knobs
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, BinarySequencer::CLOCKRATE_PARAM, -3.0f, 7.0f, 1.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, BinarySequencer::LAG_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, BinarySequencer::LAGSHAPE_PARAM, 0.0f, 1.0f, 0.0f));
	
		// scale switch
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, BinarySequencer::SCALE_PARAM, 0.0f, 2.0f, 0.0f));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, BinarySequencer::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, BinarySequencer::RESET_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, BinarySequencer::CLOCK_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, BinarySequencer::TRIGGER_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, BinarySequencer::CV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, BinarySequencer::INV_OUTPUT));
		
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, BinarySequencer::CLOCK_LIGHT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelBinarySequencer = Model::create<BinarySequencer, BinarySequencerWidget>("Count Modula", "BinarySequencer", "Binary Sequencer", SEQUENCER_TAG, CLOCK_TAG);
