//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Event timer
//	A count down timer to generate events at a specific moment
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------

struct STRUCT_NAME : Module {
	enum ParamIds {
		ENUMS(UP_PARAMS, NUM_DIGITS),
		ENUMS(DN_PARAMS, NUM_DIGITS),
		TRIGGER_PARAM,
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		TRIGGER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		END_OUTPUT,
		ENDT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		END_LIGHT,
		ENDT_LIGHT,
		ENUMS(UP_PARAM_LIGHTS, NUM_DIGITS),
		ENUMS(DN_PARAM_LIGHTS, NUM_DIGITS),
		TRIGGER_PARAM_LIGHT,
		RESET_PARAM_LIGHT,
		RETRIGGER_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gpTrigger;
	GateProcessor gpClock;
	GateProcessor gpReset;

	dsp::PulseGenerator pgEnd;
	
	bool upButtonStatus[NUM_DIGITS] = {};
	bool dnButtonStatus[NUM_DIGITS] = {};
	
	int count = 0; 
	int displayCount = 0; 
	int length = 0;
	bool retrigger = false;
	
	bool running = false;
	bool end = false;
	
	float currentTime = 0.0f;
	int processCount = 8;
	
	int mulitpliers[NUM_DIGITS] = {};
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		char buttonText[20];
		int l = 1;
		for(int i = NUM_DIGITS-1; i >=0 ; i--) {
			mulitpliers[i] = l;
			sprintf(buttonText, "Increment %d's", l);
			configButton(UP_PARAMS + i, buttonText);
			sprintf(buttonText, "Decrement %d's", l);
			configButton(DN_PARAMS + i, buttonText);
			
			l = l * 10;
		}
		
		configButton(TRIGGER_PARAM, "Manual trigger");
		configButton(RESET_PARAM, "Manual reset");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(TRIGGER_INPUT, "Trigger");

		configOutput(END_OUTPUT, "End gate");
		configOutput(ENDT_OUTPUT, "End trigger");

		displayCount = count;

		processCount = 8;

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		gpClock.reset();
		gpReset.reset();
		gpTrigger.reset();
		
		pgEnd.reset();
		
		displayCount = count = 0;
		length = 0;
		running = false;
		processCount = 8;
		currentTime = 0.0f;
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "length", json_integer(length));
		json_object_set_new(root, "count", json_integer(count));
		json_object_set_new(root, "running", json_boolean(running));
		json_object_set_new(root, "clockState", json_boolean(gpClock.high()));
		json_object_set_new(root, "retrigger", json_boolean(retrigger));
	
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		
		json_t *cnt = json_object_get(root, "count");
		json_t *len = json_object_get(root, "length");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "running");
		json_t *rt = json_object_get(root, "retrigger");

		if (cnt)
			count = json_integer_value(cnt);
		
		if (len)
			length = json_integer_value(len);
		
		if (clk)
			gpClock.preset(json_boolean_value(clk));

		if (run) 
			running = json_boolean_value(run);

		if (rt) 
			retrigger = json_boolean_value(rt);

		displayCount = count;

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
	}
	
	void process(const ProcessArgs &args) override {

		// process the buttons - can do at less than audio rate.
		if (++processCount > 8) {
			processCount = 0;
			int m = 0;
			int c = count;
			count = 0;
			bool updateValue = false;
			int n = 0;
			bool bu, bd;
			for (int i = 0; i < NUM_DIGITS; i ++) {
				m = mulitpliers[i];
				bu = params[UP_PARAMS + i].getValue() > 0.5f;
				bd = params[DN_PARAMS + i].getValue() > 0.5f;
				
				n = c / m % 10;
				
				if (bu && !upButtonStatus[i]) {
					if (++n > 9)
						n = 9;
					
					updateValue = true;
				}
				else if (bd && !dnButtonStatus[i]) {
					if (--n < 0)
						n = 0;
					
					updateValue = true;
				}

				count += n * m;
				
				upButtonStatus[i] = bu;
				dnButtonStatus[i] = bd;
			}
		
			// save new length if it was changed
			if (updateValue)
				length = count;
			
			
			// set teh retrigger mode light here
			lights[RETRIGGER_LIGHT].setBrightness(boolToLight(retrigger));			
		}
		
		// process trigger input/button
		float trigger = inputs[TRIGGER_INPUT].getNormalVoltage(0.0f);
		if (params[TRIGGER_PARAM].getValue() > 0.5f)
			trigger = 10.0f;
		
		// process reset input/button
		float reset = inputs[RESET_INPUT].getNormalVoltage(0.0f);
		if (params[RESET_PARAM].getValue() > 0.5f)
			reset = 10.0f;

		// process reset/trigger values
		gpTrigger.set(trigger);
		gpReset.set(reset);
		
		// reset on reset leading edge 
		if (gpReset.leadingEdge()) {
			// reset the count to the beginning, stop running and reset the end flag
			count = length;
			running = false;
			end = false;

			// restart internal clock
			currentTime = 0.0f;
		}

		// set running flag on trigger leading edge
		if (gpTrigger.leadingEdge()) {
			if (!end) {
				if (!running)
					currentTime = 0.0f;
				
				if(retrigger) {
					count = length;
					currentTime = 0.0f;
				}
				
				running = true;
			}
		}
		
		// process the internal clock
		currentTime += args.sampleTime;
		float v = 0.0f;
		if (currentTime > 1.0f) {
			v = 10.0f;
			currentTime -= 1.0f;
		}
		else if (currentTime < 0.5f) {
			v = 10.0f;
		}
		
		// process the clock input/internal clock
		bool clock = gpClock.set(inputs[CLOCK_INPUT].getNormalVoltage(v));
		
		// decrement count on positive clock edge if we're running
		if (gpClock.leadingEdge() && running){
			
			if (--count < 0)
				count = 0;		
		}
		
		// update the display
		displayCount = count;
			
		// set trigger pulse if we are done
		bool trigOut = false;
		if (running && count == 0) {
			running = false;
			end = true;
			trigOut = true;
			pgEnd.trigger(1e-3f);
		}
		else
			trigOut = pgEnd.process(args.sampleTime);
		
		// set the outputs and indicators
		if (end) {
			outputs[END_OUTPUT].setVoltage(10.0f);
			lights[END_LIGHT].setBrightness(1.0f);
		}
		else{
			outputs[END_OUTPUT].setVoltage(0.0f);
			lights[END_LIGHT].setBrightness(0.0f);
		}
		
		if (trigOut) {
			outputs[ENDT_OUTPUT].setVoltage(10.0f);
			lights[ENDT_LIGHT].setBrightness(1.0f);	
		}
		else {
			outputs[ENDT_OUTPUT].setVoltage(0.0f);
			lights[ENDT_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);	
		}

		// and finally the clock indicator`
		lights[CLOCK_LIGHT].setBrightness(boolToLight(clock));
	}
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	CountModulaLEDDisplayLarge *divDisplay;
	
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		float centrePos = box.size.x/2.0f;
		
		// up/down button
		float offset = 26.0f;
		float col = centrePos - ((float)NUM_DIGITS * offset / 2.0f) + offset / 2.0f;
		for (int i = 0; i < NUM_DIGITS; i ++) {
			addParam(createParamCentered<CountModulaLEDPushButtonMiniMomentary<CountModulaPBLight<GreenLight>>>(Vec(col, STD_ROWS6[STD_ROW1] - 7), module, STRUCT_NAME::UP_PARAMS + i, STRUCT_NAME::UP_PARAM_LIGHTS + i));
			addParam(createParamCentered<CountModulaLEDPushButtonMiniMomentary<CountModulaPBLight<GreenLight>>>(Vec(col, STD_ROWS6[STD_ROW2] + 6), module, STRUCT_NAME::DN_PARAMS + i, STRUCT_NAME::DN_PARAM_LIGHTS + i));
			col += offset;
		}
		
		float col1 = centrePos - 30;
		float col3 = centrePos + 30;
		
		// clock, trigger and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(col1, STD_ROWS6[STD_ROW3]), module, STRUCT_NAME::TRIGGER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(col1, STD_ROWS6[STD_ROW4]), module, STRUCT_NAME::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(col1, STD_ROWS6[STD_ROW5]), module, STRUCT_NAME::CLOCK_INPUT));
	
		// trigger and reset buttons
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(col3, STD_ROWS6[STD_ROW3]), module, STRUCT_NAME::TRIGGER_PARAM, STRUCT_NAME::TRIGGER_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(col3, STD_ROWS6[STD_ROW4]), module, STRUCT_NAME::RESET_PARAM, STRUCT_NAME::RESET_PARAM_LIGHT));
	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(col1, STD_ROWS6[STD_ROW6]), module, STRUCT_NAME::END_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(col3, STD_ROWS6[STD_ROW6]), module, STRUCT_NAME::ENDT_OUTPUT));
		
		// lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(col1 + 18, STD_ROWS6[STD_ROW5]-19), module, STRUCT_NAME::CLOCK_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(col1 + 13, STD_ROWS6[STD_ROW6] - 19), module, STRUCT_NAME::END_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(col3 + 13, STD_ROWS6[STD_ROW6] - 19), module, STRUCT_NAME::ENDT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(col3, STD_ROWS6[STD_ROW5]), module, STRUCT_NAME::RETRIGGER_LIGHT));
	
		// LED display
		divDisplay = new CountModulaLEDDisplayLarge(NUM_DIGITS);
		divDisplay->setCentredPos(Vec(box.size.x/2, STD_HALF_ROWS6(STD_ROW1)));
		divDisplay->setText(0);
		addChild(divDisplay);
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"


	// retrigger selection menu item
	struct RetrigMenuItem : MenuItem {
		STRUCT_NAME *module;
		
		void onAction(const event::Action &e) override {
			module->retrigger = !module->retrigger;
		}
	};


	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Settings"));
		
		// add the retrigger mode selection menu
		RetrigMenuItem *retrigMenuItem = createMenuItem<RetrigMenuItem>("Retrigger", CHECKMARK(module->retrigger));
		retrigMenuItem->module = module;
		menu->addChild(retrigMenuItem);
	}	
	
	void step() override {
		if (module) {
			// process any change in count
			if (module)
				divDisplay->setText(((STRUCT_NAME *)(module))->displayCount);

			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
