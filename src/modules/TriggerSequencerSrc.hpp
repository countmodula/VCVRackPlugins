//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Trigger Sequencer Module
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

	enum ParamIds {
		ENUMS(STEP_PARAMS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		ENUMS(LENGTH_PARAMS, TRIGSEQ_NUM_ROWS),
		ENUMS(CV_PARAMS, TRIGSEQ_NUM_ROWS),
		ENUMS(MUTE_PARAMS, TRIGSEQ_NUM_ROWS * 2),
		ENUMS(TRIGGER_PARAMS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		ENUMS(GATE_PARAMS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
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
		ENUMS(MUTE_PARAM_LIGHTS, TRIGSEQ_NUM_ROWS * 2),
		ENUMS(TRIGGER_PARAM_LIGHTS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		ENUMS(GATE_PARAM_LIGHTS, TRIGSEQ_NUM_STEPS * TRIGSEQ_NUM_ROWS),
		NUM_LIGHTS
	};
	
	int moduleVersion = 2;
	
	GateProcessor gateClock[TRIGSEQ_NUM_ROWS];
	GateProcessor gateReset[TRIGSEQ_NUM_ROWS];
	GateProcessor gateRun[TRIGSEQ_NUM_ROWS];
	dsp::PulseGenerator pgClock[TRIGSEQ_NUM_ROWS * 2];
		
	int count[TRIGSEQ_NUM_ROWS] = {}; 
	int length[TRIGSEQ_NUM_ROWS] = {};
	bool running[TRIGSEQ_NUM_ROWS] = {};
	
	float lengthCVScale = (float)(TRIGSEQ_NUM_STEPS - 1);
	
	int startUpCounter = 0;
	
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	SequencerExpanderMessage rightMessages[2][1]; // messages to right module (expander)
#endif
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"

	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		std::vector<std::string> channelLabels[TRIGSEQ_NUM_ROWS] = {{"A", "B"}, {"C", "D"}, {"E", "F"}, {"G", "H"}};
		std::vector<std::string> outputSelectLabels[TRIGSEQ_NUM_ROWS] = {{"B", "Off", "A"}, {"D", "Off", "C"}, {"F", "Off", "E"}, {"H", "Off", "G"}};

		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			
			// length & CV parms
			configParam(LENGTH_PARAMS + r, 1.0f, (float)(TRIGSEQ_NUM_STEPS), (float)(TRIGSEQ_NUM_STEPS), rack::string::f( "Channel %d length", r + 1));
			
			// row switches
			for (int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
				configSwitch(STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s, 0.0f, 2.0f, 0.0f, rack::string::f("Step %d select", s + 1), outputSelectLabels[r]);
				configSwitch(TRIGGER_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s, 0.0f, 1.0f, 1.0f, rack::string::f("Step %d trigger %s select", s + 1, channelLabels[r][0].c_str()), {"Off", "On"});
				configSwitch(GATE_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s, 0.0f, 1.0f, 0.0f, rack::string::f("Step %d trigger %s select", s + 1, channelLabels[r][1].c_str()), {"Off", "On"});
			}
			
			// output lights, mute buttons and jacks
			for (int i = 0; i < 2; i++) {
				configButton(MUTE_PARAMS + (r * 2) + i, rack::string::f("Mute output %s", channelLabels[r][i].c_str()));
				configOutput(TRIG_OUTPUTS + (r * 2) + i, rack::string::f("Trigger %s", channelLabels[r][i].c_str()));
			}
			
			configInput(RUN_INPUTS + r, rack::string::f("Channel %d run", r + 1));
			configInput(CLOCK_INPUTS + r, rack::string::f("Channel %d clock", r + 1));
			configInput(RESET_INPUTS + r, rack::string::f("Channel %d reset", r + 1));
			configInput(CV_INPUTS + r, rack::string::f("Channel %d length CV", r + 1));
			if (r > 0) {
				inputInfos[RUN_INPUTS + r]->description = rack::string::f("Normalled to channel %d run input", r);
				inputInfos[CLOCK_INPUTS + r]->description = rack::string::f("Normalled to channel %d clock input", r);
				inputInfos[RESET_INPUTS + r]->description = rack::string::f("Normalled to channel %d reset input", r);
			}
		}
		
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];
#endif			

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(moduleVersion));
		
		json_t *currentStep = json_array();
		json_t *clk = json_array();
		json_t *run = json_array();
	
		for (int i = 0; i < TRIGSEQ_NUM_ROWS; i++) {
			json_array_insert_new(currentStep, i, json_integer(count[i]));
			json_array_insert_new(clk, i, json_boolean(gateClock[i].high()));
			json_array_insert_new(run, i, json_boolean(gateRun[i].high()));
		}
		
		json_object_set_new(root, "currentStep", currentStep);
		json_object_set_new(root, "clockState", clk);
		json_object_set_new(root, "runState", run);		
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		json_t *version = json_object_get(root, "moduleVersion");
		json_t *currentStep = json_object_get(root, "currentStep");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "runState");
		
		if (version)
			moduleVersion = json_number_value(version);				
		else {
			// no version in file is really old version.
			moduleVersion = 0;
		}	
		
		for (int i = 0; i < TRIGSEQ_NUM_ROWS; i++) {
			if (currentStep) {
				json_t *v = json_array_get(currentStep, i);
				if (v)
					count[i] = json_integer_value(v);
			}
			
			if (clk) {
				json_t *v = json_array_get(clk, i);
				if (v)
					gateClock[i].preset(json_boolean_value(v));
			}
			
			if (run) {
				json_t *v = json_array_get(run, i);
				if (v)
					gateRun[i].preset(json_boolean_value(v));
				
				running[i] = gateRun[i].high();
			}
		}
		
		// conversion to new step select switches
		if (moduleVersion < 2) {
			INFO("Converting from module version %d", moduleVersion);
			
			for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {			
				for(int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
					int x = (int)(params[STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s].getValue());

					params[GATE_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s].setValue(x == 0 ? 1.0f : 0.0f);
					params[TRIGGER_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s].setValue(x == 2 ? 1.0f : 0.0f);
				}
			}
			
			moduleVersion = 2;
		}		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		startUpCounter = 20;		
	}	
	
	void onReset() override {
		
		for (int i = 0; i < TRIGSEQ_NUM_ROWS; i++) {
			gateClock[i].reset();
			gateReset[i].reset();
			gateRun[i].reset();
			pgClock[i].reset();
			count[i] = 0;
			length[i] = TRIGSEQ_NUM_STEPS;
		}
	}

	void process(const ProcessArgs &args) override {

		// wait a number of cycles before we use the clock and run inputs to allow them propagate correctly after startup
		if (startUpCounter > 0)
			startUpCounter--;
		
		// grab all the input values up front
		float reset = 0.0f;
		float run = 10.0f;
		float clock = 0.0f;
		float f;
		
		bool gateOutputs[SEQUENCER_EXP_NUM_TRIGGER_OUTS] = {};
		
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			// reset input
			f = inputs[RESET_INPUTS + r].getNormalVoltage(reset);
			gateReset[r].set(f);
			reset = f;
			
			if (startUpCounter == 0) {
				// run input
				f = inputs[RUN_INPUTS + r].getNormalVoltage(run);
				gateRun[r].set(f);
				run = f;

				// clock
				f = inputs[CLOCK_INPUTS + r].getNormalVoltage(clock); 
				gateClock[r].set(f);
				clock = f;
			}
			
			// sequence length - jack overrides knob
			if (inputs[CV_INPUTS + r].isConnected()) {
				// scale the input such that 10V = step 16, 0V = step 1
				length[r] = (int)(clamp(lengthCVScale/10.0f * inputs[CV_INPUTS + r].getVoltage(), 0.0f, lengthCVScale)) + 1;
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

			// process the clock trigger - we'll use this to allow the run input edge to act like the clock if it arrives shortly after the clock edge
			bool clockEdge = gateClock[r].leadingEdge();
			if (clockEdge)
				pgClock[r].trigger(1e-4f);
			else if (pgClock[r].process(args.sampleTime)) {
				// if within cooey of the clock edge, run or reset is treated as a clock edge.
				clockEdge = (gateRun[r].leadingEdge() || gateReset[r].leadingEdge());
			}
			
			if (gateRun[r].low())
				running[r] = false;
			
			// advance count on positive clock edge or the run edge if it is close to the clock edge
			if (clockEdge && gateRun[r].high()) {
				
				// flag that we are now actually running
				running[r] = true;
				
				count[r]++;
					
				if (count[r] > length[r])
					count[r] = 1;
			}
			
			// now process the lights and outputs
			bool outA = false, outB = false;
			for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
				// set step lights here
				bool stepActive = (c + 1 == count[r]);
				lights[STEP_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + c].setBrightness(boolToLight(stepActive));
				
				// now determine the output values	
				if (stepActive) {
					if(params[TRIGGER_PARAMS + (r * TRIGSEQ_NUM_STEPS) + c].getValue() > 0.5f) {
						outA = true;
					}
					
					if(params[GATE_PARAMS + (r * TRIGSEQ_NUM_STEPS) + c].getValue() > 0.5f) {
						outB = true;
					}				
				}
			}

			// save the gates for passing across the gate expander later
			gateOutputs[r * 2] = outA && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f);
			gateOutputs[(r * 2) + 1] = outB && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f);
					
			// outputs follow clock width
			outA &= (running[r] && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2)].getValue() < 0.5f));
			outB &= (running[r] && gateClock[r].high() && (params[MUTE_PARAMS + (r * 2) + 1].getValue() < 0.5f));
			
			// set the outputs accordingly
			outputs[TRIG_OUTPUTS + (r * 2)].setVoltage(boolToGate(outA));	
			outputs[TRIG_OUTPUTS + (r * 2) + 1].setVoltage(boolToGate(outB));
			lights[TRIG_LIGHTS + (r * 2)].setBrightness(boolToLight(outA));	
			lights[TRIG_LIGHTS + (r * 2) + 1].setBrightness(boolToLight(outB));
		}
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS	
		// set up details for the expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerExpanderMessage *messageToExpander = (SequencerExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				// set any potential expander module's channel number
				messageToExpander->setAllChannels(0);

				// standard number of channels = 4
				int j = 0;
				for (int i = 0; i < SEQUENCER_EXP_MAX_CHANNELS; i++) {
					messageToExpander->counters[i] = count[j];
					messageToExpander->clockStates[i] =	gateClock[j].high();
					messageToExpander->runningStates[i] = gateRun[j].high();
					
					// in case we ever add less than the expected number of rows, wrap them around to fill the expected buffer size
					if (++j == TRIGSEQ_NUM_ROWS)
						j = 0;
				}
				
				// set the gate values for the gate expander
				for (int i = 0; i < SEQUENCER_EXP_NUM_TRIGGER_OUTS; i++) {
					messageToExpander->gateStates[i] = gateOutputs[i];
				}
	
				// finally, let all potential expanders know where we came from
				if (TRIGSEQ_NUM_STEPS == 16)
					messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_TRIGGER16;
				else
					messageToExpander->masterModule = SEQUENCER_EXP_MASTER_MODULE_TRIGGER8;
					
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}		
#endif		
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	const std::vector<std::string> channelLabels[TRIGSEQ_NUM_ROWS] = {{"A", "B"}, {"C", "D"}, {"E", "F"}, {"G", "H"}};

	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;
		
		// set panel based on current default
		#include "../themes/setPanel.hpp"	

		// screws
		#include "../components/stdScrews.hpp"	
		
		// add everything row by row
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
					addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 1:
					addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 2:
					addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
				case 3:
					addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_PARAMS + r));
					break;
			}
			
			// row lights and switches
			int col;
			for (int s = 0; s < TRIGSEQ_NUM_STEPS; s++) {
				col = STD_COLUMN_POSITIONS[STD_COL6 + s] -15;
				addChild(createLightCentered<SmallLight<RedLight>>(Vec(col - 5, STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::STEP_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + s));
				addChild(createLightCentered<SmallLight<GreenLight>>(Vec(col + 5, STD_HALF_ROWS8(STD_ROW1 + (r * 2))), module, STRUCT_NAME::LENGTH_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + s));
				//addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + s] - 15, STD_ROWS8[STD_ROW2 + (r * 2)] + rowOffset), module, STRUCT_NAME:: STEP_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s));
				
				addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<RedLight>>>(Vec(col, STD_ROWS8[STD_ROW1 + (r * 2)]), module, STRUCT_NAME:: TRIGGER_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s, STRUCT_NAME:: TRIGGER_PARAM_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + s));
				addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(col, STD_ROWS8[STD_ROW2 + (r * 2)]), module, STRUCT_NAME:: GATE_PARAMS + (r * TRIGSEQ_NUM_STEPS) + s, STRUCT_NAME:: GATE_PARAM_LIGHTS + (r * TRIGSEQ_NUM_STEPS) + s));
			}
			
			// output lights, mute buttons and jacks
			for (int j = 0; j < 2; j++) {
				addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS], STD_ROWS8[STD_ROW1 + (r * 2) + j]), module, STRUCT_NAME::MUTE_PARAMS + + (r * 2) + j, STRUCT_NAME::MUTE_PARAM_LIGHTS + + (r * 2) + j));
				if (j == 0)
					addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 1], STD_ROWS8[STD_ROW1 + (r * 2) + j]), module, STRUCT_NAME::TRIG_LIGHTS + (r * 2) + j));
				else
					addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 1], STD_ROWS8[STD_ROW1 + (r * 2) + j]), module, STRUCT_NAME::TRIG_LIGHTS + (r * 2) + j));
				
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6 + TRIGSEQ_NUM_STEPS + 2], STD_ROWS8[STD_ROW1 + (r * 2) + j]), module, STRUCT_NAME::TRIG_OUTPUTS + (r * 2) + j));
			}
		}
	}
	
	struct InitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		int which = 0;

		void onAction(const event::Action &e) override {
			
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			switch (which) {
				case 0:
					h->name = rack::string::f("initialize channel %d", channel + 1);
					break;
				case 1:
					h->name = rack::string::f("initialize channel %d %s triggers", channel + 1, widget->channelLabels[channel][0].c_str());
					break;
				case 2:
					h->name = rack::string::f("initialize channel %d %s triggers", channel + 1, widget->channelLabels[channel][1].c_str());
					break;
			}
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			switch (which) {
				case 0: // entire channel
				
					// length param
					widget->getParam(STRUCT_NAME::LENGTH_PARAMS + channel)->getParamQuantity()->reset();
					
					// triggers
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
						widget->getParam(STRUCT_NAME::GATE_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
					}
					break;
				case 1:
					// A triggers only
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
					}
					break;
				case 2:
					// B triggers only
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::GATE_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->reset();
					}
					break;
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};	
	
	struct RandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		int which = 0;
	
		void onAction(const event::Action &e) override {
		
			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			switch (which) {
				case 0:
					h->name = rack::string::f("randomize channel %d", channel + 1);
					break;
				case 1:
					h->name = rack::string::f("randomize channel %d %s triggers", channel + 1, widget->channelLabels[channel][0].c_str());
					break;
				case 2:
					h->name = rack::string::f("randomize channel %d %s triggers", channel + 1, widget->channelLabels[channel][1].c_str());
					break;
			}
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();

			switch (which) {
				case 0: // entire channel
				
					// length param
					widget->getParam(STRUCT_NAME::LENGTH_PARAMS + channel)->getParamQuantity()->randomize();
					
					// triggers
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
						widget->getParam(STRUCT_NAME::GATE_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
					}
				break;
				case 1:
					// A triggers only
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::TRIGGER_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
					}
					break;
				case 2:
					// B triggers only
					for (int c = 0; c < TRIGSEQ_NUM_STEPS; c++) {
						widget->getParam(STRUCT_NAME::GATE_PARAMS + (channel * TRIGSEQ_NUM_STEPS) + c)->getParamQuantity()->randomize();
					}
					break;
			}

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};
		
	struct ChannelInitMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
	
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// full channel init
			InitMenuItem *initMenuItem = createMenuItem<InitMenuItem>("Entire Channel");
			initMenuItem->channel = channel;
			initMenuItem->widget = widget;
			initMenuItem->which = 0;
			menu->addChild(initMenuItem);

			// trigger A only init
			InitMenuItem *initTrigAMenuItem = createMenuItem<InitMenuItem>(rack::string::f("%s Triggers Only", widget->channelLabels[channel][0].c_str()));
			initTrigAMenuItem->channel = channel;
			initTrigAMenuItem->widget = widget;
			initTrigAMenuItem->which = 1;
			menu->addChild(initTrigAMenuItem);
			
			// trigger B only init
			InitMenuItem *initTrigBMenuItem = createMenuItem<InitMenuItem>(rack::string::f("%s Triggers Only", widget->channelLabels[channel][1].c_str()));
			initTrigBMenuItem->channel = channel;
			initTrigBMenuItem->widget = widget;
			initTrigBMenuItem->which = 2;
			menu->addChild(initTrigBMenuItem);			
			
			return menu;
		}
	
	};
	
	struct ChannelRandMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// full channel random
			RandMenuItem *randMenuItem = createMenuItem<RandMenuItem>("Entire Channel");
			randMenuItem->channel = channel;
			randMenuItem->which = 0;
			randMenuItem->widget = widget;
			
			menu->addChild(randMenuItem);

			// trigger A only random
			RandMenuItem *randTrigAMenuItem = createMenuItem<RandMenuItem>(rack::string::f("%s Triggers Only", widget->channelLabels[channel][0].c_str()));
			randTrigAMenuItem->channel = channel;
			randTrigAMenuItem->widget = widget;
			randTrigAMenuItem->which = 1;
			menu->addChild(randTrigAMenuItem);
			
			// trigger B only random
			RandMenuItem *randTrigBMenuItem = createMenuItem<RandMenuItem>(rack::string::f("%s Triggers Only", widget->channelLabels[channel][1].c_str()));
			randTrigBMenuItem->channel = channel;
			randTrigBMenuItem->widget = widget;
			randTrigBMenuItem->which = 2;
			menu->addChild(randTrigBMenuItem);			
			
			return menu;
		}	
	};
	
	struct ChannelMenuItem : MenuItem {
		WIDGET_NAME *widget;
		int channel = 0;

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// initialize
			ChannelInitMenuItem *initMenuItem = createMenuItem<ChannelInitMenuItem>("Initialize", RIGHT_ARROW);
			initMenuItem->channel = channel;
			initMenuItem->widget = widget;
			menu->addChild(initMenuItem);

			// randomize
			ChannelRandMenuItem *randMenuItem = createMenuItem<ChannelRandMenuItem>("Randomize", RIGHT_ARROW);
			randMenuItem->channel = channel;
			randMenuItem->widget = widget;
			menu->addChild(randMenuItem);

			return menu;
		}
	};
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

#ifdef SEQUENCER_EXP_MAX_CHANNELS	
	// expander addition menu item
	#include "../inc/AddExpanderMenuItem.hpp"	
#endif

	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());

		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Sequence"));
		char textBuffer[100];
		for (int r = 0; r < TRIGSEQ_NUM_ROWS; r++) {
			
			sprintf(textBuffer, "Channel %d", r + 1);
			ChannelMenuItem *chMenuItem = createMenuItem<ChannelMenuItem>(textBuffer, RIGHT_ARROW);
			chMenuItem->channel = r;
			chMenuItem->widget = this;
			menu->addChild(chMenuItem);
		}
		
#ifdef SEQUENCER_EXP_MAX_CHANNELS			
		// add expander menu
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Expansion"));

		AddExpanderMenuItem *gateMenuItem = createMenuItem<AddExpanderMenuItem>("Add gate expander");
		gateMenuItem->module = module;
		gateMenuItem->model = modelSequencerExpanderTSG;
		gateMenuItem->immediateRight =true;
		gateMenuItem->position = box.pos.plus(math::Vec(box.size.x,0));
		gateMenuItem->expanderName = "gate";
		menu->addChild(gateMenuItem);		
		
#if TRIGSEQ_NUM_STEPS == 8
		AddExpanderMenuItem *cvMenuItem = createMenuItem<AddExpanderMenuItem>("Add CV expander");
		cvMenuItem->module = module;
		cvMenuItem->model = modelSequencerExpanderCV8;
		cvMenuItem->position = box.pos;
		cvMenuItem->expanderName = "CV";
		menu->addChild(cvMenuItem);	
		
		AddExpanderMenuItem *outputMenuItem = createMenuItem<AddExpanderMenuItem>("Add output expander");
		outputMenuItem->module = module;
		outputMenuItem->model = modelSequencerExpanderOut8;
		outputMenuItem->position = box.pos;
		outputMenuItem->expanderName = "output";
		menu->addChild(outputMenuItem);		
		
		AddExpanderMenuItem *trigMenuItem = createMenuItem<AddExpanderMenuItem>("Add trigger expander");
		trigMenuItem->module = module;
		trigMenuItem->model = modelSequencerExpanderTrig8;
		trigMenuItem->position = box.pos;
		trigMenuItem->expanderName = "trigger";
		menu->addChild(trigMenuItem);			
		
		AddExpanderMenuItem *melodyMenuItem = createMenuItem<AddExpanderMenuItem>("Add random melody expander");
		melodyMenuItem->module = module;
		melodyMenuItem->model = modelSequencerExpanderRM8;
		melodyMenuItem->position = box.pos;
		melodyMenuItem->expanderName = "random melody";
		menu->addChild(melodyMenuItem);	

		AddExpanderMenuItem *logicMenuItem = createMenuItem<AddExpanderMenuItem>("Add logic expander");
		logicMenuItem->module = module;
		logicMenuItem->model = modelSequencerExpanderLog8;
		logicMenuItem->position = box.pos;
		logicMenuItem->expanderName = "logic";
		menu->addChild(logicMenuItem);			
#endif
#endif	
	}
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
