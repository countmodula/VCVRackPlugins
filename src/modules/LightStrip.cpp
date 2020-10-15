//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Light strip
// LED Light strip blank module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME LightStrip
#define PANEL_FILE "LightStrip.svg"

struct LightStrip : Module {
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
		ENUMS(STRIP_LIGHT, 3),
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	float rValue = 0.294f;
	float gValue = 0.635999858f;
	float bValue = 0.0f;
	
	float prValue = 0.0f;
	float pgValue = 0.0f;
	float pbValue = 0.0f;
	
	float rRevert, gRevert, bRevert;
	
	bool narrowMode = false;
	bool prevMode = false;
	
	void saveRevertValues() {
		rRevert = rValue;
		gRevert = gValue;
		bRevert = bValue;
	}
	
	void restoreRevertValues() {
		rValue = rRevert;
		gValue = gRevert;
		bValue = bRevert;
	}
	
	ModuleLightWidget* lightStripWidget;
	float widePos;

	// read the default color value from the global count modula settings file
	void readDefaultColor() {
		
		// default to the, um, default colour...
		rValue = 0.294f;
		gValue = 0.635999858f;
		bValue = 0.0f;		
		
		// read the settings file
		json_t *rootJ = readSettings();
		
		// read the default color details
		json_t* color = json_object_get(rootJ, "lightStripDefaultColor");
		double r, g, b;
		if (color) {
			json_unpack(color, "[f, f, f]", &r, &g, &b);
			rValue = r;
			gValue = g;
			bValue = b;
		}		

		// houskeeping
		json_decref(rootJ);
	}

	// save the given color value in the global count modula settings file
	void saveDefaultColor() {
		// read the settings file
		json_t *rootJ = readSettings();
		
		// set the default color value
		json_t* color = json_pack("[f, f, f]", rValue, gValue, bValue);
		json_object_set_new(rootJ, "lightStripDefaultColor", color);
		
		// save the updated data
		saveSettings(rootJ);
		
		// houskeeping
		json_decref(rootJ);
	}	
				
	bool readDefaultSize() {
		// default to wide
		bool narrow = false;	
		
		// read the settings file
		json_t *rootJ = readSettings();
		
		// read the default size details
		json_t *defSize = json_object_get(rootJ, "lightStripNarrowIsDefault");
		if (defSize) {
			narrow = json_boolean_value(defSize);
		}		
	
		// houskeeping
		json_decref(rootJ);
		
		return narrow;
	}
	
	// save the given color value in the global count modula settings file
	void saveDefaultSize(bool narrow) {
		// read the settings file
		json_t *rootJ = readSettings();
		
		// set the default size value
		json_object_set_new(rootJ, "lightStripNarrowIsDefault", json_boolean(narrow));
		narrowMode = narrow;
	
		// save the updated data
		saveSettings(rootJ);
		
		// houskeeping
		json_decref(rootJ);
	}			
		
	LightStrip() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		readDefaultColor();
		narrowMode = readDefaultSize();

	}

	void onReset() override {
		readDefaultColor();
	}
	
	void onRandomize() override {
		rValue = clamp(random::uniform(), 0.0f, 1.0f);
		gValue = clamp(random::uniform(), 0.0f, 1.0f);
		bValue = clamp(random::uniform(), 0.0f, 1.0f);
	}

	json_t *dataToJson() override {
		json_t *root = json_object();
		
		json_object_set_new(root, "moduleVersion", json_integer(1));
			
		json_t* color = json_pack("[f, f, f]", rValue, gValue, bValue);
		json_object_set_new(root, "color", color);
				
		json_object_set_new(root, "narrowMode", json_boolean(narrowMode));				
				
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		json_t* color = json_object_get(root, "color");
		double r, g, b;
		if (color) {
			json_unpack(color, "[f, f, f]", &r, &g, &b);
			rValue = r;
			gValue = g;
			bValue = b;
		}
		
		json_t *stripSize = json_object_get(root, "narrowMode");
		if (stripSize) {
			narrowMode = json_boolean_value(stripSize);
		}			
		
		prevMode = !narrowMode;
	}
	
	void process(const ProcessArgs &args) override {
		
		if (prValue != rValue || pgValue != gValue || pbValue != bValue)
		{
			lights[STRIP_LIGHT].setBrightness(rValue); 		// red
			lights[STRIP_LIGHT + 1].setBrightness(gValue);	// green
			lights[STRIP_LIGHT + 2].setBrightness(bValue); 	// blue
			
			prValue = rValue;
			pgValue = gValue;
			pbValue = bValue;
		}
	}
};

struct LightStripWidget : ModuleWidget {
	//----------------------------------------------------------------
	// custom lights 
	//----------------------------------------------------------------
	// special black background RGB light
	template <typename TBase = app::ModuleLightWidget>
	struct TBlackRedGreenBlueLight : TBase {
		TBlackRedGreenBlueLight() {
			this->bgColor = nvgRGB(0, 0, 0); // black when all leds off
			this->borderColor = nvgRGBA(0, 0, 0, 0x60);
			
			this->addBaseColor(nvgRGB(0xff, 0x00, 0x00)); // bright red
			this->addBaseColor(nvgRGB(0x00, 0xff, 0x00)); // bright green
			this->addBaseColor(nvgRGB(0x00, 0x00, 0xff)); // bright blue
		}
	};
	typedef TBlackRedGreenBlueLight<> BlackRedGreenBlueLight;

	// special tall thin strip light
	template <typename TBase>
	struct CountModulaLightStrip : TBase {
		CountModulaLightStrip() {
			this->box.size = app::mm2px(math::Vec(3.176, 115.0));
		}
	};

	//----------------------------------------------------------------
	// colour menu stuff
	//----------------------------------------------------------------
	struct ColorQuantity : Quantity {
		float *colorValue;
		float defaultValue;
		std::string text;
		
		ColorQuantity(std::string label, float *value, float defValue) {
			defaultValue = clamp(defValue, 0.0f, 1.0f);
			colorValue = value;
			defValue = defValue;
			text = label;
		}
		
		void setValue(float value) override {
			*colorValue = clamp(value, 0.0f, 1.0f);
		}
		float getValue() override {
			return *colorValue;
		}
		std::string getLabel() override {
			return text;
		}
		float getDisplayValue() override {
			return getValue() * 100;
		}
		void setDisplayValue(float displayValue) override {
			setValue(displayValue / 100);
		}	
		float getDefaultValue() override {
			return defaultValue;
		}
		std::string getUnit() override {
			return "%";
		}
	};

	struct ColorSlider : ui::Slider {
		ColorSlider(std::string label, float *value, float defValue) {
			quantity = new ColorQuantity(label, value, defValue);
			this->box.size.x = 200.0;
		}
		~ColorSlider() {
			delete quantity;
		}
	};

	// revert menu item
	struct RevertMenuItem : MenuItem {
		LightStrip *module;
		float *rOrig, *gOrig, *bOrig;
		void onAction(const event::Action &e) override {
			module->restoreRevertValues();
		}
	};	

	// colour menu item
	struct ColorSliderMenu : MenuItem {
		LightStrip *module;
		
		Menu *createChildMenu() override {
			
			Menu *menu = new Menu;
			
			menu->addChild(createMenuLabel("Adjust Colour"));
			ColorSlider* rSlider = new ColorSlider("Red", &(module->rValue), module->rValue);
			menu->addChild(rSlider);	
			
			ColorSlider* gSlider = new ColorSlider("Green", &(module->gValue), module->gValue);
			menu->addChild(gSlider);	
			
			ColorSlider* bSlider = new ColorSlider("Blue", &(module->bValue), module->bValue);
			menu->addChild(bSlider);

			RevertMenuItem* revertMenuItem = createMenuItem<RevertMenuItem>("Revert Changes");
			revertMenuItem->module = module;
			menu->addChild(revertMenuItem);

			return menu;	
		}
	};

	// default colour setting menu item
	struct DefaultColorMenuItem : MenuItem {
		LightStrip *module;
		bool save = true;
		
		void onAction(const event::Action &e) override {
			if (save)
				module->saveDefaultColor();
			else
			{
				module->readDefaultColor();
				module->saveRevertValues();
			}
		}
	};

	struct ColorMenu : MenuItem {
		LightStrip *module;
		
		Menu *createChildMenu() override {
			
			module->saveRevertValues();
			
			Menu *menu = new Menu;

			// set default colour
			DefaultColorMenuItem *setDefaultColorMenuItem = createMenuItem<DefaultColorMenuItem>("Save as default");
			setDefaultColorMenuItem->module = module;
			setDefaultColorMenuItem->save = true;
			menu->addChild(setDefaultColorMenuItem);
			
			// get default colour
			DefaultColorMenuItem *getDefaultColorMenuItem = createMenuItem<DefaultColorMenuItem>("Revert to default");
			getDefaultColorMenuItem->module = module;
			getDefaultColorMenuItem->save = false;
			menu->addChild(getDefaultColorMenuItem);	
		
			// colour adjust
			ColorSliderMenu *colorMenu = createMenuItem<ColorSliderMenu>("Select Colour", RIGHT_ARROW);
			colorMenu->module = module;
			menu->addChild(colorMenu);
		
			return menu;	
		}
	};

	//----------------------------------------------------------------
	// strip size menu stuff
	//----------------------------------------------------------------

	//strip size menu item
	struct StripSizeMenuItem : MenuItem {
		LightStrip *module;
		
		void onAction(const event::Action &e) override {
			module->narrowMode = !module->narrowMode;
		}
	};

	// strip size default menu item
	struct DefaultStripSizeMenuItem : MenuItem {
		LightStrip *module;
		bool value;
		void onAction(const event::Action &e) override {
			module->saveDefaultSize(value);
		}
	};

	struct StripSizeMenu : MenuItem {
		LightStrip *module;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// strip size menu
			StripSizeMenuItem *stripSizeMenuItem = createMenuItem<StripSizeMenuItem>("Narrow Strip", CHECKMARK(module->narrowMode));
			stripSizeMenuItem->module = module;
			menu->addChild(stripSizeMenuItem);
			
			// default strip size
			bool narrow = module->readDefaultSize();
			DefaultStripSizeMenuItem *defaultStripSizeMenuItem = createMenuItem<DefaultStripSizeMenuItem>("Narrow Strip As Default", CHECKMARK(narrow));
			defaultStripSizeMenuItem->module = module;
			defaultStripSizeMenuItem->value = !narrow;
			menu->addChild(defaultStripSizeMenuItem);
		
			return menu;	
		}
	};
//----------------------------------------------------------------

	std::string panelName;
	
	LightStripWidget(LightStrip *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// light strip
		if (module) {
			module->lightStripWidget = createLightCentered<RectangleLight<CountModulaLightStrip<BlackRedGreenBlueLight>>>(Vec(RACK_GRID_WIDTH / 2.0, STD_ROWS5[STD_ROW3]), module, LightStrip::STRIP_LIGHT);
			module->widePos = module->lightStripWidget->box.pos.x;
		
			addChild(module->lightStripWidget);
		}
		else
			addChild(createLightCentered<RectangleLight<CountModulaLightStrip<BlackRedGreenBlueLight>>>(Vec(RACK_GRID_WIDTH / 2.0, STD_ROWS5[STD_ROW3]), module, LightStrip::STRIP_LIGHT));
		
		// screws
		#include "../components/stdScrews.hpp"	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		LightStrip *module = dynamic_cast<LightStrip*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	
		// colour menu
		StripSizeMenu *sizeMenu = createMenuItem<StripSizeMenu>("Strip Size", RIGHT_ARROW);
		sizeMenu->module = module;
		menu->addChild(sizeMenu);
		
		// colour menu
		ColorMenu *colorMenu = createMenuItem<ColorMenu>("Strip Colour", RIGHT_ARROW);
		colorMenu->module = module;
		menu->addChild(colorMenu);
	}
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
			
			if (((LightStrip*)module)->narrowMode != ((LightStrip*)module)->prevMode)
			{
				ModuleLightWidget* lightStripWidget = ((LightStrip*)module)->lightStripWidget;
				
				if (((LightStrip*)module)->narrowMode) {
					lightStripWidget->box.size = app::mm2px(math::Vec(1.5, 115.0));
					lightStripWidget->box.pos.x = 5.25;
				}
				else {
					lightStripWidget->box.size = app::mm2px(math::Vec(3.176, 115.0));
					lightStripWidget->box.pos.x = ((LightStrip*)module)->widePos; 
				}
				
				((LightStrip*)module)->prevMode = ((LightStrip*)module)->narrowMode;
			}
		}
		
		Widget::step();
	}	
};	

Model *modelLightStrip = createModel<LightStrip, LightStripWidget>("LightStrip");
