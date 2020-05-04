//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Blank Panel Functionality
//	Blank Panel
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BlankPanel

struct BlankPanel : Module {
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

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	BlankPanel() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));

		// add the theme details
		#include "../themes/dataToJson.hpp"
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
};

struct WIDGET_NAME : ModuleWidget {
	WIDGET_NAME(BlankPanel *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" PANEL_FILE)));

		// screws
		#include "../components/stdScrews.hpp"	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		BlankPanel *module = dynamic_cast<BlankPanel*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
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

Model *MODEL_NAME = createModel<BlankPanel, WIDGET_NAME>(MODULE_NAME);
