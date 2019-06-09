//----------------------------------------------------------------------------
//	Count Modula - Trigger Sequencer Module
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "dsp/digital.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define TRIGSEQ_NUM_ROWS	4
#define TRIGSEQ_NUM_STEPS	16 

struct TriggerSequencer16 : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		ENUMS(LENGTH_PARAMS, TRIGSEQ_NUM_ROWS),
		ENUMS(CV_PARAMS, TRIGSEQ_NUM_ROWS),
		ENUMS(MUTE_PARAMS, TRIGSEQ_NUM_ROWS * 2),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(RUN_INPUTS, TRIGSEQ_NUM_ROWS),
		ENUMS(CLOCK_INPUTS, TRIGSEQ_NUM_ROWS),
		ENUMS(RESET_INPUTS, TRIGSEQ_NUM_ROWS),
		ENUMS(CV_INPUTS, TRIGSEQ_NUM_ROWS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUTS, TRIGSEQ_NUM_ROWS * 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		ENUMS(TRIG_LIGHTS, TRIGSEQ_NUM_ROWS * 2),
		NUM_LIGHTS
	};
	
	GateProcessor gateClock[TRIGSEQ_NUM_ROWS];
	GateProcessor gateReset[TRIGSEQ_NUM_ROWS];
	GateProcessor gateRun[TRIGSEQ_NUM_ROWS];
	PulseGenerator pgTrig[TRIGSEQ_NUM_ROWS * 2];
	
	int count[TRIGSEQ_NUM_ROWS]; 
	int length[TRIGSEQ_NUM_ROWS];
	
	float cvScale = (float)(TRIGSEQ_NUM_STEPS - 1);
	
	TriggerSequencer16() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	void onReset() override {
		
		for (int i = 0; i < TRIGSEQ_NUM_ROWS; i++) {
			gateClock[i].reset();
			gateReset[i].reset();
			gateRun[i].reset();
			count[i] = 0;
			length[i] = TRIGSEQ_NUM_STEPS;
		}
		for (int i = 0; i < TRIGSEQ_NUM_ROWS * 2; i++) {
			pgTrig[i].reset();
		}
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void TriggerSequencer16::step() {

	// grab all the input values up front
	float reset = 0.0f;
	float run = 10.0f;
	float clock = 0.0f;
	float f;
	for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
		// reset input
		f = inputs[RESET_INPUTS + r].normalize(reset);
		gateReset[r].set(f);
		reset = f;
		
		// run input
		f = inputs[RUN_INPUTS + r].normalize(run);
		gateRun[r].set(f);
		run = f;

		// clock
		f = inputs[CLOCK_INPUTS + r].normalize(clock); 
		gateClock[r].set(f);
		clock = f;
		
		// sequence length - jack overrides knob
		if (inputs[CV_INPUTS + r].active) {
			// scale the input such that 10V = step 16, 0V = step 1
			length[r] = (int)(clamp(cvScale/10.0f * inputs[CV_INPUTS + r].value, 0.0f, cvScale)) + 1;
		}
		else {
			length[r] = (int)(params[LENGTH_PARAMS + r].value);
		}
	}

	// now process the steps for each row as required
	for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
		if (gateReset[r].leadingEdge()) {
			// reset the count at zero
			count[r] = 0;
		}

		bool running = gateRun[r].high();
		if (running) {
			// increment count on positive clock edge
			if (gateClock[r].leadingEdge()){
				count[r]++;
				
				if (count[r] > length[r])
					count[r] = 1;
			}
		}
		
		// now process the lights and outputs
		bool outA = false, outB = false;
		for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
			// set step lights here
			bool stepActive = (c + 1 == count[r]);
			lights[STEP_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + c].setBrightness(boolToLight(stepActive));
			
			// now determine the output values
			if (stepActive) {
				switch ((int)(params[STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + c].value)) {
					case 0: // lower output
						outA = false;
						outB = true;
						break;
					case 2: // upper output
						outA = true;
						outB = false;
						break;				
					default: // off
						outA = false;
						outB = false;
						break;
				}
			}
		}

		// outputs follow clock width
		outA &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].value < 0.5f));
		outB &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2) + 1].value < 0.5f));
		
		// set the outputs accordingly
		outputs[TRIG_OUTPUTS + (r * 2)].value = boolToGate(outA);	
		outputs[TRIG_OUTPUTS + (r * 2) + 1].value = boolToGate(outB);
		lights[TRIG_LIGHTS + (r * 2)].setBrightness(boolToLight(outA));	
		lights[TRIG_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(outB));
	}
}

struct TriggerSequencer16Widget : ModuleWidget {
	TriggerSequencer16Widget(TriggerSequencer16 *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/TriggerSequencer16.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// add everything row by row
		int rowOffset = -10;
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			// run input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (r * 2)]), module, TriggerSequencer16::RUN_INPUTS + r));

			// reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (r * 2)]), module, TriggerSequencer16::RESET_INPUTS + r));
			
			// clock input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3]-15, STD_ROWS8[STD_ROW1 + (r * 2)]), module, TriggerSequencer16::CLOCK_INPUTS + r));
					
			// CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3]-15, STD_ROWS8[STD_ROW2 + (r * 2)]), module, TriggerSequencer16::CV_INPUTS + r));
			
			// length & CV params
			switch (r % 4) {
				case 0:
					addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, TriggerSequencer16::LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS)));
					break;
				case 1:
					addParam(createParamCentered<CountModulaRotarySwitchGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, TriggerSequencer16::LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS)));
					break;
				case 2:
					addParam(createParamCentered<CountModulaRotarySwitchYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, TriggerSequencer16::LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS)));
					break;
				case 3:
					addParam(createParamCentered<CountModulaRotarySwitchBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, TriggerSequencer16::LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS)));
					break;
			}
			
			// row lights and switches
			int i = 0;
			for (int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s] - 15, STD_ROWS8[STD_ROW1 + (r * 2)] + rowOffset), module, TriggerSequencer16::STEP_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + i));
				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s] - 15, STD_ROWS8[STD_ROW2 + (r * 2)] + rowOffset), module, TriggerSequencer16:: STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + i++, 0.0f, 2.0f, 1.0f));
			}
			
			// output lights, mute buttons and jacks
			for (int i = 0; i < 2; i++) {
				addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, TriggerSequencer16::MUTE_PARAMS + + (r * 2) + i, 0.0f, 1.0f, 0.0f));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 1], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, TriggerSequencer16::TRIG_LIGHTS + (r * 2) + i));
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 2], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, TriggerSequencer16::TRIG_OUTPUTS + (r * 2) + i));
			}
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelTriggerSequencer16 = Model::create<TriggerSequencer16, TriggerSequencer16Widget>("Count Modula", "TriggerSequencer16", "Trigger Sequencer (16 Steps)", SEQUENCER_TAG);
