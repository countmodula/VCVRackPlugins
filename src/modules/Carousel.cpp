//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Carousel
//	A rotating router/switch
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Carousel
#define PANEL_FILE "Carousel.svg"

struct Carousel : Module {
	enum ParamIds {
		ROTATE_UP_PARAM,
		ROTATE_DN_PARAM,
		RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(CV_INPUT, 8),
		ROTATE_UP_INPUT,
		ROTATE_DN_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CV_OUTPUT, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECT_LIGHT, 8),
		ENUMS(ACTIVE_LIGHT, 8),
		ROTATE_UP_PARAM_LIGHT,
		ROTATE_DN_PARAM_LIGHT,
		RESET_PARAM_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gpUp;
	GateProcessor gpDn;
	GateProcessor gpReset;
	
	int offset = 0, prevOffset = 0;
	int processCount = 0;
	int nextMaxChan = 0, maxChans = 7, prevMaxChans = -1;
	bool inactivePassthrough = false;
	
	int outputMap [8][16] = {	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{ 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{ 0, 1, 2, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{ 0, 1, 2, 3, 0, 1, 2, 3, 0, 0 ,0 ,0 ,0 ,0 ,0 ,0},
								{ 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0},
								{ 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 0, 0, 0},
								{ 0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6, 0, 0}, 
								{ 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7} };
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	Carousel() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(ROTATE_UP_PARAM, 0.0, 1.0, 0.0, "Rotate up");
		configParam(ROTATE_DN_PARAM, 0.0, 1.0, 0.0, "Rotate down");
		configParam(RESET_PARAM, 0.0, 1.0, 0.0, "Reset");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"	
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "offset", json_integer(offset));
		json_object_set_new(root, "activeInputs", json_integer(nextMaxChan));
		json_object_set_new(root, "inactivePassthrough", json_boolean(inactivePassthrough));
		
		// TODO: save the current output voltages
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		
		json_t *sel = json_object_get(root, "offset");
		json_t *ai = json_object_get(root, "activeInputs");
		json_t *ip = json_object_get(root, "inactivePassthrough");

		if (sel)
			offset = json_integer_value(sel);	
		
		if (ai)
			nextMaxChan = json_integer_value(ai);			
		else
			nextMaxChan = 7;
			
			
		if (ip)
			offset = json_boolean_value(ip);
			
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}

	void onReset() override {
		prevOffset = offset;
		offset = 0;
		nextMaxChan = 0;
		inactivePassthrough = false;
	}
	
	void process(const ProcessArgs &args) override {
	
		if (++processCount > 4) {
			
			maxChans = nextMaxChan;
			
			// basing the number of chans on the connected inputs)
			if (maxChans < 1) {
				for(int i = 0; i < 8; i++) {
					if (inputs[CV_INPUT + i].isConnected())
						maxChans = i;
				}
			}
			
			gpUp.set(params[ROTATE_UP_PARAM].getValue() > 0.5f ? 10.0f : inputs[ROTATE_UP_INPUT].getVoltage());
			gpDn.set(params[ROTATE_DN_PARAM].getValue() > 0.5f ? 10.0f : inputs[ROTATE_DN_INPUT].getVoltage());
			gpReset.set(params[RESET_PARAM].getValue() > 0.5f ? 10.0f : inputs[RESET_INPUT].getVoltage());
		
			if (gpUp.leadingEdge()) {
				if (--offset < 0)
					offset = maxChans;
			}
			
			if (gpDn.leadingEdge()) {
				if (++offset > maxChans)
					offset = 0;
			}
			
			if (gpReset.leadingEdge())
				offset = 0;
				
			processCount = 0;	
		}
		
		if (offset > maxChans)
			 offset = maxChans;

		lights[SELECT_LIGHT + offset].setBrightness(1.0f);
		
		for (int i = 0; i < 8; i++) {
			if (i > maxChans) {
				// when out of the selected range, just pass the input through to the output
				outputs[CV_OUTPUT + i].setVoltage(inactivePassthrough ? inputs[CV_INPUT + i].getVoltage() : 0.0f);
			}
			else
				outputs[CV_OUTPUT + outputMap[maxChans][offset +i]].setVoltage(inputs[CV_INPUT + i].getVoltage());
		}
		
		// turn off previous selected input indicator if we need to
		if (prevOffset != offset) {
			lights[SELECT_LIGHT + prevOffset].setBrightness(0.0f);
			prevOffset = offset;
		}
		
		if (maxChans != prevMaxChans) {
			for (int i = 0; i < 8; i ++)
				lights[ACTIVE_LIGHT + i].setBrightness(boolToLight(i <= maxChans));
				
			prevMaxChans = maxChans;
		}
		
	}
};

struct CarouselWidget : ModuleWidget {
	std::string panelName;

	CarouselWidget(Carousel *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"

		for (int i = 0; i < 8; i++) {
			// addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 20, STD_ROWS8[i] + 8), module, Carousel::ACTIVE_LIGHT + i));
			addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] - 9, STD_ROWS8[i]), module, Carousel::ACTIVE_LIGHT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[i]), module, Carousel::CV_INPUT + i));
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 4, STD_ROWS8[i]), module, Carousel::SELECT_LIGHT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[i]), module, Carousel::CV_OUTPUT + i));
		}
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW2]), module, Carousel::ROTATE_UP_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW4]), module, Carousel::ROTATE_DN_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW6]), module, Carousel::RESET_INPUT));
		
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW3]), module, Carousel::ROTATE_UP_PARAM, Carousel::ROTATE_UP_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW5]), module, Carousel::ROTATE_DN_PARAM, Carousel::ROTATE_DN_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMomentary<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS8[STD_ROW7]), module, Carousel::RESET_PARAM, Carousel::RESET_PARAM_LIGHT));
			
		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	// active input selection menu item
	struct ActiveInputMenuItem : MenuItem {
		Carousel *module;
		int value;
		
		void onAction(const event::Action& e) override {
			module->nextMaxChan = value;
		}
	};
	
	// active input selection menu
	struct ActiveInputMenu : MenuItem {
		Carousel *module;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			char textBuffer[100];
			for (int i = 0; i < 8; i++) {
				if (i < 1)
					sprintf(textBuffer, "Determine by connected inputs");
				else
					sprintf(textBuffer, "%d", i + 1);
				
				ActiveInputMenuItem *ailMenuItem = createMenuItem<ActiveInputMenuItem>(textBuffer, CHECKMARK(i == module->nextMaxChan));
				ailMenuItem->module = module;
				ailMenuItem->value = i;
				menu->addChild(ailMenuItem);
			}
			
			return menu;
		}
	};

	// inactive input passthrough mode
	struct InActivePassThroughMenuItem : MenuItem {
		Carousel *module;
	
		void onAction(const event::Action& e) override {
			module->inactivePassthrough ^= true;
		}
	};


	void appendContextMenu(Menu *menu) override {
		Carousel *module = dynamic_cast<Carousel*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// active input selection menu
		ActiveInputMenu *aiMenuItem = createMenuItem<ActiveInputMenu>("Active Routes", RIGHT_ARROW);
		aiMenuItem->module = module;	
		menu->addChild(aiMenuItem);		
		
		// inactive input passthrough menu item
		InActivePassThroughMenuItem *iaptlMenuItem = createMenuItem<InActivePassThroughMenuItem>("Inactive Routes Passthrough", CHECKMARK(module->inactivePassthrough));
		iaptlMenuItem->module = module;
		menu->addChild(iaptlMenuItem);		
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};


Model *modelCarousel = createModel<Carousel, CarouselWidget>("Carousel");
