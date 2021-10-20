//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Event timer
// A count down timer to generate events at a specific moment
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME EventTimer
#define PANEL_FILE "EventTimer.svg"

struct EventTimer : Module {
	enum ParamIds {
		ENUMS(UP_PARAMS, 3),
		ENUMS(DN_PARAMS, 3),
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
		ENUMS(UP_PARAM_LIGHTS, 3),
		ENUMS(DN_PARAM_LIGHTS, 3),
		TRIGGER_PARAM_LIGHT,
		RESET_PARAM_LIGHT,
		NUM_LIGHTS
	};

	CountModulaLEDDisplayLarge3 *divDisplay;
		
	char lengthString[4];
	GateProcessor gpTrigger;
	GateProcessor gpClock;
	GateProcessor gpReset;

	dsp::PulseGenerator pgEnd;
	
	bool upButtonStatus[3] = {};
	bool dnButtonStatus[3] = {};
	
	int count = 0; 
	int displayCount = 0; 
	int length = 0;

	bool update = true;
	bool running = false;
	bool end = false;
	
	float currentTime = 0.0f;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	EventTimer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		char buttonText[20];
		int l = 100;
		for(int i = 0; i < 3; i++) {
			sprintf(buttonText, "Increment %d's", l);
			configButton(UP_PARAMS + i, buttonText);
			sprintf(buttonText, "Decrement %d's", l);
			configButton(DN_PARAMS + i, buttonText);
			
			l = l / 10;
		}
		
		configButton(TRIGGER_PARAM, "Manual trigger");
		configButton(RESET_PARAM, "Manual reset");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(TRIGGER_INPUT, "Trigger");

		configOutput(END_OUTPUT, "End gate");
		configOutput(ENDT_OUTPUT, "End trigger");

		displayCount = count;

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
		update = true;
		
		currentTime = 0.0f;
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "length", json_integer(length));
		json_object_set_new(root, "count", json_integer(count));
		json_object_set_new(root, "running", json_boolean(running));
		json_object_set_new(root, "clockState", json_boolean(gpClock.high()));
	
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		
		json_t *cnt = json_object_get(root, "count");
		json_t *len = json_object_get(root, "length");
		json_t *clk = json_object_get(root, "clockState");
		json_t *run = json_object_get(root, "running");

		if (cnt)
			count = json_integer_value(cnt);
		
		if (len)
			length = json_integer_value(len);
		
		if (clk)
			gpClock.preset(json_boolean_value(clk));

		if (run) 
			running = json_boolean_value(run);

		update = false;
		displayCount = count;

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
	}
	
	void process(const ProcessArgs &args) override {

		// process the buttons
		int m = 100;
		int c = count;
		count = 0;
		bool updateValue = false;
		int n = 0;
		bool bu, bd;
		for (int i = 0; i < 3; i ++) {
			bu = params[UP_PARAMS + i].getValue() > 0.5f;
			bd = params[DN_PARAMS + i].getValue() > 0.5f;
			
			n = c / m % 10;
			
			if (bu && !upButtonStatus[i]) {
				if (++n > 9)
					n = 9;
				
				updateValue = update = true;
			}
			else if (bd && !dnButtonStatus[i]) {
				if (--n < 0)
					n = 0;
				
				updateValue = update = true;
			}

			count += n * m;
			m = m  / 10;
			
			upButtonStatus[i] = bu;
			dnButtonStatus[i] = bd;
		}
	
		// save new length if it was changed
		if (updateValue)
			length = count;
		
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
			
			// make sure we update the display
			update = true;
		}

		// set running flag on trigger leading edge
		if (!end && gpTrigger.leadingEdge()) {
			if (!running)
				currentTime = 0.0f;
			
			running = true;
		}
		
		// process the internal clock
		currentTime += args.sampleTime;
		float v = 0.0f;
		if (currentTime > 1.0f) {
			v = 10.0f;
			currentTime = 0.0f;
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

			// make sure we update the display
			update = true;			
		}
		
		// update the display
		if (update) {
			displayCount = count;
			
			// reset update flag so we don't update the display until actually need to
			update = false;
		}
		
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

struct EventTimerWidget : ModuleWidget {

	std::string panelName;
	CountModulaLEDDisplayLarge3 *divDisplay;
	
	EventTimerWidget(EventTimer *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// up/down buttons
		int offset[3] = {4, 0, -4};
		for (int i = 0; i < 3; i ++) {
			addParam(createParamCentered<CountModulaLEDPushButtonMiniMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[i] + offset[i], STD_ROWS6[STD_ROW1] - 7), module, EventTimer::UP_PARAMS + i, EventTimer::UP_PARAM_LIGHTS + i));
			addParam(createParamCentered<CountModulaLEDPushButtonMiniMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[i] + offset[i], STD_ROWS6[STD_ROW2] + 6), module, EventTimer::DN_PARAMS + i, EventTimer::DN_PARAM_LIGHTS + i));
		}
		
		// clock, trigger and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, EventTimer::TRIGGER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, EventTimer::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, EventTimer::CLOCK_INPUT));
	
		// trigger and reset buttons
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, EventTimer::TRIGGER_PARAM, EventTimer::TRIGGER_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, EventTimer::RESET_PARAM, EventTimer::RESET_PARAM_LIGHT));
	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, EventTimer::END_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, EventTimer::ENDT_OUTPUT));
		
		// lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 18, STD_ROWS6[STD_ROW5]-19), module, EventTimer::CLOCK_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 13, STD_ROWS6[STD_ROW6] - 19), module, EventTimer::END_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 13, STD_ROWS6[STD_ROW6] - 19), module, EventTimer::ENDT_LIGHT));
	
		// LED display
		divDisplay = new CountModulaLEDDisplayLarge3();
		divDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_HALF_ROWS6(STD_ROW1)));
		divDisplay->text =  "000";
		addChild(divDisplay);
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		EventTimer *module = dynamic_cast<EventTimer*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change in count
			if (module)
				divDisplay->text = rack::string::f("%03d", ((EventTimer *)(module))->displayCount);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelEventTimer = createModel<EventTimer, EventTimerWidget>("EventTimer");
