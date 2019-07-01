//----------------------------------------------------------------------------
//	/^M^\ Count Modula - XOR Logic Gate Module
//	A 1-8 8-1 multiplexer module / 1 to 8 router / 8 to 1 switcher
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"

struct Multiplexer : Module {
	enum ParamIds {
		LENGTH_S_PARAM,
		LENGTH_R_PARAM,
		HOLD_PARAM,
		NORMAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_S_INPUT,
		CLOCK_R_INPUT,
		RESET_S_INPUT,
		RESET_R_INPUT,
		LENGTH_S_INPUT,
		LENGTH_R_INPUT,
		SEND_INPUT,
		ENUMS(RECEIVE_INPUTS, 8),
		NUM_INPUTS
	};	
	enum OutputIds {
		RECEIVE_OUTPUT,
		ENUMS(SEND_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SEND_LIGHTS, 8),
		ENUMS(RECEIVE_LIGHTS, 8),
		NUM_LIGHTS
	};

	enum NormallingMode {
		INPUT_MODE,
		ZERO_MODE,
		ASSOC_MODE,
		MULTI_MODE
	};
	
	enum sampleMode {
		TRACKANDHOLD_MODE,
		TRACKONLY_MODE,
		SAMPLEANDHOLD_MODE
	};
	
	int indexS = -1;
	int indexR = -1;
	
	GateProcessor gateClockS;
	GateProcessor gateClockR;
	GateProcessor gateResetS;
	GateProcessor gateResetR;
	
	bool linkedClock = true;
	
	int lengthS = 7;
	int lengthR = 7;
	int normallingMode = 1;
	int holdMode = 1;
	
	float sendOutputs[8];
	
	Multiplexer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(LENGTH_S_PARAM, 1.0f, 7.0f, 7.0f, "Number of router steps (Sends)");
		configParam(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Router sample mode");
		
		configParam(LENGTH_R_PARAM, 1.0f, 8.0f, 8.0f, "Number of selector steps (Receives)");
		configParam(NORMAL_PARAM, 1.0f, 4.0f, 1.0f, "Selector normalling mode");
	}
	
	void onReset() override {
		indexS = -1;
		indexR = -1;
		lengthS = 7;
		lengthR = 7;
		
		gateClockS.reset();
		gateClockR.reset();
		gateResetS.reset();
		gateResetR.reset();	
	}
	
	void process(const ProcessArgs &args) override {
		
		// flag if the send clock also clocks the receives
		linkedClock = !inputs[CLOCK_R_INPUT].isConnected();
		
		// what hold and normalling modes are selected?
		normallingMode = ((int)(params[NORMAL_PARAM].getValue())) - 1;
		holdMode = (int)(params[HOLD_PARAM].getValue());
		bool doSample = holdMode != SAMPLEANDHOLD_MODE;
		
		// determine the sequence lengths
		lengthS = (int)(params[LENGTH_S_PARAM].getValue());
		lengthR = (int)(params[LENGTH_R_PARAM].getValue());
		float cvS = clamp(inputs[LENGTH_S_INPUT].getVoltage(), 0.0f, 10.0f);
		float cvR = clamp(inputs[LENGTH_R_INPUT].getVoltage(), 0.0f, 10.0f);
		
		// cv overrides switch position;
		if(inputs[LENGTH_S_INPUT].isConnected()) {
			// scale cv input to 1-7
			lengthS = (int)(1.0f + (cvS * 0.6f));
		}

		if(inputs[LENGTH_R_INPUT].isConnected()) {
			// scale cv input to 1-7
			lengthR = (int)(1.0f + (cvR * 0.6f));
		}

		// sync to send has been selected
		if (lengthR > 7)
			lengthR = lengthS;
		
		// handle the reset inputs
		float resetS = inputs[RESET_S_INPUT].getNormalVoltage(0.0f);
		float resetR = inputs[RESET_R_INPUT].getNormalVoltage(resetS);
		gateResetS.set(resetS);
		gateResetR.set(resetR);
		
		// grab the clock input values
		
		float clockS = inputs[CLOCK_S_INPUT].getVoltage();
		float clockR = linkedClock ? clockS : inputs[CLOCK_R_INPUT].getVoltage();
		gateClockS.set(clockS);
		gateClockR.set(clockR);
		
		// send reset/clock logic
		if (gateResetS.high()) {
			// stop all outputs and reset to start
			indexS = -1;
		}
		else {
			// advance S clock if required
			if (gateClockS.leadingEdge()) {
				indexS = (indexS < lengthS ? indexS + 1 : 0);
				doSample = true;	
			}
		}
		
		// receive reset/clock logic
		if (gateResetR.high()) {
			// stop all outputs and reset to start
			indexR = -1;
		}
		else {
			if ((linkedClock && gateClockS.leadingEdge()) || (!linkedClock && gateClockR.leadingEdge())) {
				// advance R clock if required
				indexR = (indexR < lengthR ? indexR + 1 : 0);
			}	
		}

		// grab the send input  value	
		float sendIn = inputs[SEND_INPUT].getVoltage();
		
		// now send the input to the appropriate send output and the appropriate receive input to the output
		for (int i = 0; i < 8; i ++) {
			switch (holdMode) {
				case SAMPLEANDHOLD_MODE:
					if (doSample) {						
						sendOutputs[i] = (i == indexS ? sendIn : sendOutputs[i]);
						outputs[SEND_OUTPUTS + i].setVoltage(i == indexS ? sendIn : sendOutputs[i]);
					}
					break;
				case TRACKANDHOLD_MODE:
						sendOutputs[i] = (i == indexS ? sendIn : sendOutputs[i]);
						outputs[SEND_OUTPUTS + i].setVoltage(i == indexS ? sendIn : sendOutputs[i]);
					break;
				case TRACKONLY_MODE:
				default:
					sendOutputs[i] = (i == indexS ? sendIn : 0.0f);
					outputs[SEND_OUTPUTS + i].setVoltage(i == indexS ? sendIn : 0.0f);
					break;
			}
			lights[SEND_LIGHTS + i].setBrightness(i == indexS ? 1.0f : 0.0f);
		}
		
		for (int i = 0; i < 8; i ++) {
			// receives
			if (i == indexR) {
				float normalVal = 0.0f;
				switch (normallingMode) {
					case MULTI_MODE:
						normalVal = sendOutputs[indexS];
						break;
					case ASSOC_MODE:
						normalVal = sendOutputs[i];
						break;
					case INPUT_MODE:
						normalVal = sendIn;
						break;
					default:
					case ZERO_MODE:
						normalVal = 0.0f;
						break;
				}
				
				if (inputs[RECEIVE_INPUTS + i].isConnected())
					outputs[RECEIVE_OUTPUT].setVoltage(inputs[RECEIVE_INPUTS + i].getVoltage());
				else
					outputs[RECEIVE_OUTPUT].setVoltage(normalVal);

				lights[RECEIVE_LIGHTS + i].setBrightness(1.0f );
			}
			else			
				lights[RECEIVE_LIGHTS + i].setBrightness(0.0f );
		}
	}
};



struct MultiplexerWidget : ModuleWidget {
	MultiplexerWidget(Multiplexer *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Multiplexer.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//--------------------------------------------------------
		// router section
		//--------------------------------------------------------
		addParam(createParamCentered<CountModulaRotarySwitchWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW7)), module, Multiplexer::LENGTH_S_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, Multiplexer::HOLD_PARAM));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, Multiplexer::SEND_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, Multiplexer::CLOCK_S_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, Multiplexer::RESET_S_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, Multiplexer::LENGTH_S_INPUT));
			
		for (int i = 0; i < 8; i++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::SEND_LIGHTS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::SEND_OUTPUTS + i));
		}
	
		//--------------------------------------------------------
		// selector section
		//--------------------------------------------------------
		for (int i = 0; i < 8; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::RECEIVE_INPUTS + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::RECEIVE_LIGHTS + i));
		}

		addParam(createParamCentered<CountModulaRotarySwitchRed>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_HALF_ROWS8(STD_ROW2)), module, Multiplexer::LENGTH_R_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitch5PosYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_HALF_ROWS8(STD_ROW6)), module, Multiplexer::NORMAL_PARAM));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW1]), module, Multiplexer::LENGTH_R_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW4]), module, Multiplexer::CLOCK_R_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW5]), module, Multiplexer::RESET_R_INPUT));
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW8]), module, Multiplexer::RECEIVE_OUTPUT));
	}
};

Model *modelMultiplexer = createModel<Multiplexer, MultiplexerWidget>("Multiplexer");
