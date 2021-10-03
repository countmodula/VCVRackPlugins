//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - BarGraph
//	Bar graph volt meter
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BarGraph
#define PANEL_FILE "BarGraph.svg"

struct BarGraph : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	
	enum LightIds {
		ENUMS(BAR_LIGHT, 21),
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"

	float thresh[21] = {10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f, -10.0f};
	int scale = 1;
	
	BarGraph() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configInput(CV_INPUT, "Signal");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"		
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		json_object_set_new(root, "scale", json_integer(scale));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		
		json_t *sc = json_object_get(root, "scale");

		scale = 1;
		if (sc)
			scale = json_integer_value(sc);		
		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	

	void onReset() override {
		scale = 1;
	}	
	
	void process(const ProcessArgs& args) override {

		float cv = inputs[CV_INPUT].getVoltage() * (float)scale;
		
		for (int i = 0; i < 21; i++) {
			if (i < 10) {
				if (cv >= thresh[i])
					lights[i].setBrightness(1.0f);
				else 
					lights[i].setBrightness(0.0f);
			}
			else if (i > 10) {
				if (cv <= thresh[i])
					lights[i].setBrightness(1.0f);
				else 
					lights[i].setBrightness(0.0f);				
			}
			else {
				lights[i].setBrightness(1.0f);
			}
		}
	}
};

struct BarGraphWidget : ModuleWidget { 
	std::string panelName;
	
	BarGraphWidget(BarGraph *module) {
		setModule(module);

		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));
		
		// screws
		#include "../components/stdScrews.hpp"
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(22.5, STD_ROWS6[STD_ROW6]), module, BarGraph::CV_INPUT));
		
		// lights
		for (int i = 0; i < 21; i++) {
			if ( i == 10)
				addChild(createLightCentered<MediumLightRectangle<CountModulaRectangleLight<RedLight>>>(Vec(22.5, STD_ROWS6[STD_ROW1] + (i * 12)), module, BarGraph::BAR_LIGHT + i));
			else
				addChild(createLightCentered<MediumLightRectangle<CountModulaRectangleLight<GreenLight>>>(Vec(22.5, STD_ROWS6[STD_ROW1] + (i * 12)), module, BarGraph::BAR_LIGHT + i));
		}
	}
	
	//---------------------------------------------------------------------
	// scale menu stuff
	//---------------------------------------------------------------------
	struct ScaleMenuItem : MenuItem {
		BarGraph *module;
		int scale;
		
		void onAction(const event::Action& e) override {
			module->scale = scale;
		}		
	};	

	struct ScaleMenu : MenuItem {
		BarGraph *module;
	
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			// 1 volt scale  menu item
			ScaleMenuItem *sMenuItem1 = createMenuItem<ScaleMenuItem>("1 Volt", CHECKMARK(module->scale == 10));
			sMenuItem1->module = module;
			sMenuItem1->scale = 10;
			menu->addChild(sMenuItem1);	
		
			// 5 volt scale  menu item
			ScaleMenuItem *sMenuItem5 = createMenuItem<ScaleMenuItem>("5 Volts", CHECKMARK(module->scale == 2));
			sMenuItem5->module = module;
			sMenuItem5->scale = 2;
			menu->addChild(sMenuItem5);		
		
			// 10 volt scale  menu item
			ScaleMenuItem *sMenuItem10 = createMenuItem<ScaleMenuItem>("10 Volts", CHECKMARK(module->scale == 1));
			sMenuItem10->module = module;
			sMenuItem10->scale = 1;
			menu->addChild(sMenuItem10);		
		
			return menu;	
		}			
	};	

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BarGraph *module = dynamic_cast<BarGraph*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		//scale  menu
		ScaleMenu *scaleMenu = createMenuItem<ScaleMenu>("Scale", RIGHT_ARROW);
		scaleMenu->module = module;
		menu->addChild(scaleMenu);		
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}		
};


Model *modelBarGraph = createModel<BarGraph, BarGraphWidget>("BarGraph");
