//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Event Arranger Module
//	Complex event generator based on John Blacet's "VCC & Event Arranger" 
//	article published in Synapse magazine in Feb '78. 
//	With a twist...
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

struct EventArranger : Module {

	enum ParamIds {
		RESET_PARAM,
		RUN_PARAM,
		ENUMS(BIT_PARAM, 15),
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		RUN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		INV_OUTPUT,
		TRIGGER_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RUN_LIGHT,
		GATE_LIGHT,
		ENUMS(BIT_LIGHT, 15),
		NUM_LIGHTS
	};

	// how many bits do we want?
	const int NUMBITS = 15;

	// set maximum counter value based on number of bits
	long maxnum = 1UL << NUMBITS;
	
	// set bitmask starting point based on number of bits we're working with
	long bitmask = maxnum >> 1;
	
	long count = 0;
	bool out = true;
	bool currentGate = false;
	
	GateProcessor gateClock;
	GateProcessor gateReset;
	GateProcessor gateRun;
	
	dsp::PulseGenerator  pgTrig;
	
	EventArranger() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// add the bit switches
		int z;
		char bitDesc[20];
		for (int i = 0; i < 3; i++) {
			
			for (int j = 0; j < 5; j++) {
				z = (i * 5) + j;
				sprintf(bitDesc, "Bit %d Select", 14 - z);
				configParam(BIT_PARAM + z, 0.0f, 2.0f, 1.0f, bitDesc);
			}
		}
		
		// buttons
		configParam(RUN_PARAM, 0.0f, 1.0f, 1.0f, "Run");
		configParam(RESET_PARAM, 0.0f, 1.0f, 0.0f, "Reset");
	}
	
	void onReset() override {
		count = 0;
		out = true;
		currentGate = false;
		
		gateClock.reset();
		gateReset.reset();
		gateRun.reset();
	
		pgTrig.reset();
	}

	void process(const ProcessArgs &args) override {

		// process the reset input/button
		float reset = fmaxf(inputs[RESET_INPUT].getNormalVoltage(0.0f), params[RESET_PARAM].getValue() * 10.0f);
		gateReset.set(reset);
		
		// process the run input/button -- jack takes precedence over the button
		float run = inputs[RUN_INPUT].getNormalVoltage(params[RUN_PARAM].getValue() * 10.0f);
		gateRun.set(run);
		
		// process the clock
		float clock = inputs[CLOCK_INPUT].getVoltage(); 
		//stClock.process(rescale(clock, 0.1f, 2.f, 0.f, 1.f));
		gateClock.set(clock);
		
		if (gateReset.high()) {
			// hold the count at zero and stop it advancing until we release the reset
			count = 0;
		}
		else {
			if (gateRun.high())
			{
				// force a reset on positive edge of run
				if (gateRun.leadingEdge())
					count = 0;
				
				// increment count on positive clock edge
				if (gateClock.leadingEdge()){
					count++;
					
					if (count > maxnum)
						count = 0;
				}
			}
		}
		
		// reset the bit mask to the first bit on the left and 
		long b = bitmask;

		// 	start off with a clean slate for the output
		bool gotbits = false;
		out = true;
		
		// process each bit in turn
		for (int i = 0; i < NUMBITS; i++) {
			int switchValue = (int)(params[BIT_PARAM + i].getValue());
			
			// determine if current bit is set and act accordingly
			if ((count & b) == b ) {
				// current bit is high
				lights[BIT_LIGHT + i].setBrightness(1.0f);
				
				// is the switch high or any?
				if (switchValue != 1) {
					gotbits = true;
					out &= (switchValue == 2);
				}
			}
			else {
				// current bit is low
				lights[BIT_LIGHT + i].setBrightness(0.0f);
				
				// is the switch low or any?
				if (switchValue != 1) {
					gotbits = true;
					out &= (switchValue == 0);
				}
			}
			
			// shift bit mask to the next bit
			b = b >> 1;
		}
		
		// ensure we have no output if all bits are set to don't care
		out &= gotbits;
		
		// set the outputs appropriately
		if (out) {
			if (!currentGate) {
				// gate transition from 0 to 1 - kick off the trigger
				pgTrig.trigger(1e-3f);
			}
			
			outputs[GATE_OUTPUT].setVoltage(10.0f);
			outputs[INV_OUTPUT].setVoltage(0.0f);
			lights[GATE_LIGHT].setBrightness(1.0f);
		}
		else {
			outputs[GATE_OUTPUT].setVoltage(0.0f);
			outputs[INV_OUTPUT].setVoltage(10.0f);
			lights[GATE_LIGHT].setBrightness(0.0f);
		}
		
		currentGate = out;

		// set these last two here as they rely on the above logic
		outputs[TRIGGER_OUTPUT].setVoltage(boolToGate(pgTrig.process(args.sampleTime)));
		lights[RUN_LIGHT].setBrightness(gateRun.light());
	}

};


struct EventArrangerWidget : ModuleWidget {
	EventArrangerWidget(EventArranger *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/EventArranger.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// custom row positions for this module
		const int STD_ROWS[8] = {
			33,
			75,
			112,
			154,
			196,
			238,
			295,
			337
		};			
		
		// add the bit switches and LEDS
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 5; j++) {
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + (j * 2)], STD_ROWS[STD_ROW1  + (i * 2)]), module, EventArranger::BIT_LIGHT + ((i * 5) + j)));
				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1 + (j * 2)], STD_ROWS[STD_ROW2  + (i * 2)]), module, EventArranger::BIT_PARAM + ((i * 5) + j)));
			}
		}
		
		// buttons
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS[STD_ROW7]), module, EventArranger::RUN_PARAM));
		addParam(createParamCentered<CountModulaPBSwitchMomentary>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS[STD_ROW7]), module, EventArranger::RESET_PARAM));

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS[STD_ROW8]), module, EventArranger::CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS[STD_ROW8]), module, EventArranger::RUN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS[STD_ROW8]), module, EventArranger::RESET_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS[STD_ROW8]), module, EventArranger::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS[STD_ROW7]), module, EventArranger::TRIGGER_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS[STD_ROW8]), module, EventArranger::INV_OUTPUT));
	
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS[STD_ROW7]), module, EventArranger::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS[STD_ROW7]), module, EventArranger::RUN_LIGHT));
	
	}
};

Model *modelEventArranger = createModel<EventArranger, EventArrangerWidget>("EventArranger");
