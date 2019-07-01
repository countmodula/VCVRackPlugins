//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Trigger Sequencer Module
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

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
		ENUMS(LENGTH_LIGHTS,  TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		NUM_LIGHTS
	};
	
	GateProcessor gateClock[TRIGSEQ_NUM_ROWS];
	GateProcessor gateReset[TRIGSEQ_NUM_ROWS];
	GateProcessor gateRun[TRIGSEQ_NUM_ROWS];
	dsp::PulseGenerator pgTrig[TRIGSEQ_NUM_ROWS * 2];
	
	int count[TRIGSEQ_NUM_ROWS]; 
	int length[TRIGSEQ_NUM_ROWS];
	
	float cvScale = (float)(TRIGSEQ_NUM_STEPS - 1);
	
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		char rowText[20];
		
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			
			// length & CV parms
			sprintf(rowText, "Channel %d length", r + 1);
			configParam(LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS), rowText);
			
			// row lights and switches
			int i = 0;
			char stepText[20];
			for (int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
				sprintf(stepText, "Step %d select", s + 1);
				configParam(STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + i++, 0.0f, 2.0f, 1.0f, stepText);
			}
			
			// output lights, mute buttons and jacks
			for (int i = 0; i < 2; i++) {
				configParam(MUTE_PARAMS + + (r * 2) + i, 0.0f, 1.0f, 0.0f, "Mute this output");
			}
		}
	}
	
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

	void process(const ProcessArgs &args) override {

		// grab all the input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			// reset input
			f = inputs[RESET_INPUTS + r].getNormalVoltage(reset);
			gateReset[r].set(f);
			reset = f;
			
			// run input
			f = inputs[RUN_INPUTS + r].getNormalVoltage(run);
			gateRun[r].set(f);
			run = f;

			// clock
			f = inputs[CLOCK_INPUTS + r].getNormalVoltage(clock); 
			gateClock[r].set(f);
			clock = f;
			
			// sequence length - jack overrides knob
			if (inputs[CV_INPUTS + r].isConnected()) {
				// scale the input such that 10V = step 16, 0V = step 1
				length[r] = (int)(clamp(cvScale/10.0f * inputs[CV_INPUTS + r].getVoltage(), 0.0f, cvScale)) + 1;
			}
			else {
				length[r] = (int)(params[LENGTH_PARAMS + r].getValue());
			}
			
			// set the length lights
			for(int i = 0; i < TRIGSEQ_NUM_STEPS; i++) {
				lights[LENGTH_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + i].setBrightness(boolToLight(i < length[r]));
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
					switch ((int)(params[STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + c].getValue())) {
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
			outA &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			outB &= (running && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));
			
			// set the outputs accordingly
			outputs[TRIG_OUTPUTS + (r * 2)].setVoltage(boolToGate(outA));	
			outputs[TRIG_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(outB));
			lights[TRIG_LIGHTS + (r * 2)].setBrightness(boolToLight(outA));	
			lights[TRIG_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(outB));
		}
	}

};

struct WIDGET_NAME : ModuleWidget {
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, PANEL_FILE)));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// add everything row by row
		int rowOffset = -10;
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			// run input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (r * 2)]), module, STRUCT_NAME::RUN_INPUTS + r));

			// reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (r * 2)]), module, STRUCT_NAME::RESET_INPUTS + r));
			
			// clock input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3]-15, STD_ROWS8[STD_ROW1 + (r * 2)]), module, STRUCT_NAME::CLOCK_INPUTS + r));
					
			// CV input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3]-15, STD_ROWS8[STD_ROW2 + (r * 2)]), module, STRUCT_NAME::CV_INPUTS + r));
			
			// length & CV params
			switch (r % 4) {
				case 0:
					addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 1:
					addParam(createParamCentered<CountModulaRotarySwitchGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 2:
					addParam(createParamCentered<CountModulaRotarySwitchYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 3:
					addParam(createParamCentered<CountModulaRotarySwitchBlue>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
			}
			
			// row lights and switches
			int i = 0;
			for (int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s] - 15, STD_ROWS8[STD_ROW1 + (r * 2)] + rowOffset), module, STRUCT_NAME::STEP_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + i));
				addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s]- 5, STD_ROWS8[STD_ROW1 + (r * 2)] + 3), module, STRUCT_NAME::LENGTH_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + i));
				addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s] - 15, STD_ROWS8[STD_ROW2 + (r * 2)] + rowOffset), module, STRUCT_NAME:: STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + i++));
			}
			
			// output lights, mute buttons and jacks
			for (int i = 0; i < 2; i++) {
				addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, STRUCT_NAME::MUTE_PARAMS + + (r * 2) + i));
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 1], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, STRUCT_NAME::TRIG_LIGHTS + (r * 2) + i));
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 2], STD_ROWS8[STD_ROW1 + (r * 2) + i]), module, STRUCT_NAME::TRIG_OUTPUTS + (r * 2) + i));
			}
		}
	}
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
