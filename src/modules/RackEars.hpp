//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Rack Ears Functionality
//	Rack Ears
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

// set the module name for the theme selection functions
#define THEME_MODULE_NAME RackEar

struct RackEar : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int panelType = 0;
	int prevPanelType = -1;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	RackEar() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the panel type from the current default
		panelType = readDefaultIntegerValue("DefaulRackEarPanelType");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "panelType", json_integer(panelType));

		// add the theme details
		#include "../themes/dataToJson.hpp"
		
		return root;
	}

	void dataFromJson(json_t* root) override {

		json_t *pt = json_object_get(root, "panelType");		
		
		panelType = 0;
		if (pt)
			panelType = json_integer_value(pt);
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
};

struct WIDGET_NAME : ModuleWidget {

	std::string panelName;
	
	WIDGET_NAME(RackEar *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));
	}

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	
	// panel selection menu item
	struct PanelMenuItem : MenuItem {
		THEME_MODULE_NAME *module;
		int panelToUse = 0;
		
		void onAction(const event::Action &e) override {
			module->panelType = panelToUse;
		}
	};
	
	// module panel menu
	struct PanelMenu : MenuItem {
		THEME_MODULE_NAME *module;
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// add Standard panel menu item
			PanelMenuItem *standardMenuItem = createMenuItem<PanelMenuItem>("Count Modula Logo", CHECKMARK(module->panelType == 0));
			standardMenuItem->module = module;
			standardMenuItem->panelToUse = 0;
			menu->addChild(standardMenuItem);
			
			// add minimalist (nologo) panel menu item
			PanelMenuItem *nologoMenuItem = createMenuItem<PanelMenuItem>("Minimalist", CHECKMARK(module->panelType == 1));
			nologoMenuItem->module = module;
			nologoMenuItem->panelToUse = 1;
			menu->addChild(nologoMenuItem);
			
			return menu;	
		}
	};	
		
	// default panel selection menu item
	struct DefaultPanelMenuItem : MenuItem {
		THEME_MODULE_NAME *module;
		int panelToUse = 0;
		
		void onAction(const event::Action &e) override {
			// write the setting
			saveDefaultIntegerValue("DefaulRackEarPanelType", panelToUse);
			
			// might as well set the current theme too
			module->panelType = panelToUse;
		}
	};		
		

	// default panel menu
	struct DefaultPanelMenu : MenuItem {
		THEME_MODULE_NAME *module;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			int currentDefault = readDefaultIntegerValue("DefaulRackEarPanelType");

			// add Standard theme menu item
			DefaultPanelMenuItem *standardMenuItem = createMenuItem<DefaultPanelMenuItem>("Count Modula Logo", CHECKMARK(currentDefault == 0));
			standardMenuItem->module = module;
			standardMenuItem->panelToUse = 0;
			menu->addChild(standardMenuItem);
			
			// add Absinthe theme menu item
			DefaultPanelMenuItem *nologoMenuItem = createMenuItem<DefaultPanelMenuItem>("Minimalist", CHECKMARK(currentDefault == 1));
			nologoMenuItem->module = module;
			nologoMenuItem->panelToUse = 1;
			menu->addChild(nologoMenuItem);
			
			return menu;	
		}
	};		
			
	void appendContextMenu(Menu *menu) override {
		RackEar *module = dynamic_cast<RackEar*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add panel selection menu item
		PanelMenu *panelMenuItem = createMenuItem<PanelMenu>("Ear Type", RIGHT_ARROW);
		panelMenuItem->module = module;
		menu->addChild(panelMenuItem);

		// add panel selection menu item
		DefaultPanelMenu *defaultPanelMenuItem = createMenuItem<DefaultPanelMenu>("Default Ear Type", RIGHT_ARROW);
		defaultPanelMenuItem->module = module;
		menu->addChild(defaultPanelMenuItem);
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}
	
	void step() override {
		if (module) {
			
			// process any change of logo
			int cPanel = ((THEME_MODULE_NAME*)module)->panelType;
			int pPanel = ((THEME_MODULE_NAME*)module)->prevPanelType;	
			
			if (cPanel != pPanel) {
				switch (cPanel) {
					case 1: // blank
						panelName = PANEL_FILE1;
						break;
					
					default: // anything else = Count Modula Logo
						panelName = PANEL_FILE;
						break;
				}
				
				((THEME_MODULE_NAME*)module)->prevPanelType = cPanel;
				((THEME_MODULE_NAME*)module)->prevTheme =  -1; // force panel refresh
			}
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *MODEL_NAME = createModel<RackEar, WIDGET_NAME>(MODULE_NAME);

