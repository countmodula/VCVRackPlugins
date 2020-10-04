//---------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Cable Palette
//	Cable colour management tool
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#include <settings.hpp>

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Palette
#define PANEL_FILE "Palette.svg"

#define FORCE_MAX_COLOURS false
#define NUM_COLOURS 7
#define MAX_COLOURS 15

// for the custom color picker menu entry
#define BND_LABEL_FONT_SIZE 13

struct Palette;

static Palette* paletteSingleton = NULL;

std::vector<NVGcolor> cableColors = {
	nvgRGB(0x20, 0x20, 0x20),	// black (ish)
	nvgRGB(0x8b, 0x45, 0x13),	// brown
	nvgRGB(0xc9, 0x18, 0x47),	// red
	nvgRGB(0xdd, 0x6c, 0x00),	// orange
	nvgRGB(0xc9, 0xb7, 0x0e),	// yellow
	nvgRGB(0x0c, 0x8e, 0x15),	// green
	nvgRGB(0x09, 0x86, 0xad),	// blue
	nvgRGB(0x8a, 0x2b, 0xe2),	// violet
	nvgRGB(0x80, 0x80, 0x80),	// grey
	nvgRGB(0xff, 0xff, 0xff),	// white
	nvgRGB(0x4b, 0xf2, 0xed),	// cyan
	nvgRGB(0xa8, 0x32, 0x89),	// magenta
	nvgRGB(0x67, 0x73, 0x36),	// olive
	nvgRGB(0xf5, 0xa9, 0xe0)	// pink
};

std::vector<std::string> cableColorNames = {
		"Black",
		"Brown",
		"Red",
		"Orange",
		"Yellow",
		"Green",
		"Blue",
		"Violet",
		"Grey",
		"White",
		"Cyan",
		"Magenta",
		"Olive",
		"Pink"
};

struct Palette : Module {
	enum ParamIds {
		LOCK_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(SELECT_LIGHTS, MAX_COLOURS),
		LOCK_PARAM_LIGHT,
		NUM_LIGHTS
	};

	bool doChange;
	int startColorID = -1;
	int nextColorID = -1;
	int count = 0;
	
	int numColoursToUse = FORCE_MAX_COLOURS ? MAX_COLOURS : NUM_COLOURS;
	int colourRemoveID = -1;
	
	std::vector<LightWidget *> buttons;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	bool running = false;
	
	Palette() {
		
		if (paletteSingleton == NULL) {
			paletteSingleton = this;
			running = true;
		}	
		
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(LOCK_PARAM, 0.0f, 1.0f, 0.0f, "Lock current colour");

		// set the theme from the current default value
		if (running) {
			#include "../themes/setDefaultTheme.hpp"
		}
	}
	
	~Palette() {
		if (paletteSingleton == this) {
			paletteSingleton = NULL;
		}		
	}
	
	void onRandomize() override {
		if (running) {
			// randomly select a colour for the next cable
			float x = (float)(settings::cableColors.empty() ? 0: clamp(settings::cableColors.size(), 0, MAX_COLOURS));
			x = x * random::uniform();
			startColorID = (int)x;
		}
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "color", json_integer(nextColorID));	
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t *root) override {
		
		json_t *col = json_object_get(root, "color");

		startColorID = -1;
		if (col) {
			startColorID = json_integer_value(col);
			
			if (startColorID >= (int)(settings::cableColors.size()))
				startColorID = 0;
		}
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {

		if (running) {
			// continue with the colour we had chosen at the time the patch was saved 
			if (startColorID >= 0) {
				APP->scene->rack->nextCableColorId = startColorID;
				nextColorID = startColorID = -1;
				doChange = true;
			}
			
			int colorID = APP->scene->rack->nextCableColorId;
			
			// if we're locked, keep the next colour as the current colour
			if (!doChange) {
				if (colorID != nextColorID && params[LOCK_PARAM].getValue() > 0.5f) {
					if (nextColorID >= 0)
						colorID = APP->scene->rack->nextCableColorId = nextColorID;
				}
			}
			
			// update the LEDs if there's been a change
			if (count == 0 || doChange) {
				if (nextColorID != colorID) {
					for (int i = 0; i < numColoursToUse; i++) {
						lights[SELECT_LIGHTS + i].setBrightness(boolToLight(i == colorID));
					}
					
					// sync the next colour id
					nextColorID = colorID;
				}
			}
			
			// update the display every 8th cycle
			if (++count > 8)
				count = 0;
			
			doChange = false;
		}
	}
};


struct PaletteWidget : ModuleWidget {
	
	//----------------------------------------------------------------
	// colour customisation menu stuff
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
		 int getDisplayPrecision() override {
			return 3;
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

	struct ApplyMenuItem : MenuItem {
		int colorID = 0;
		NVGcolor *color;
		NVGcolor *originalColor;
		
		void onAction(const event::Action &e) override {
			*originalColor = *color;
			settings::cableColors[colorID] = *color;
		}
	};

	struct CancelMenuItem : MenuItem {
		void onAction(const event::Action &e) override {
			// no action required, just let the menu destructor do it's thing.
		}
	};

	// Custom colour menu item
	struct ColorSliderMenu : MenuItem {
		NVGcolor *color;
		int colorID;
		NVGcolor originalColor;
		bool *overrideRevert;
		Menu *createChildMenu() override {
			
			Menu *menu = new Menu;
			
			menu->addChild(createMenuLabel("Customise Colour"));
			
			// RGB sliders
			ColorSlider* rSlider = new ColorSlider("Red", &(color->r), color->r);
			ColorSlider* gSlider = new ColorSlider("Green", &(color->g), color->g);
			ColorSlider* bSlider = new ColorSlider("Blue", &(color->b), color->b);
			menu->addChild(rSlider);	
			menu->addChild(gSlider);	
			menu->addChild(bSlider);

			// menu item to apply the adjuted colour
			ApplyMenuItem* applyMenuItem = createMenuItem<ApplyMenuItem>("Apply");
			applyMenuItem->colorID = colorID;
			applyMenuItem->color = color;
			applyMenuItem->originalColor = &originalColor;
			menu->addChild(applyMenuItem);

			// menu item to cancel the colour adjustment
			CancelMenuItem* cancelMenuItem = createMenuItem<CancelMenuItem>("Cancel");
			menu->addChild(cancelMenuItem);

			return menu;	
		}
		
		~ColorSliderMenu() {
			// if we're removing the color after adjusting it, we donlt want to do this otherwise we replace the next color
			if (!*overrideRevert) {
				*color = originalColor;
				settings::cableColors[colorID] = *color;
			}
		}
	};

	//---------------------------------------------------------------------
	// colour selection menu stuff
	//---------------------------------------------------------------------
	// remove colour menu item
	struct RemoveMenuItem : MenuItem {
		Palette *module;
		int colorID = 0;
		bool* overrideRevert;
		void onAction(const event::Action &e) override {
			if (!settings::cableColors.empty()) {
				settings::cableColors.erase(settings::cableColors.begin() + colorID);
				module->colourRemoveID = colorID;
				int x = (int)(settings::cableColors.size());
				if (module->nextColorID >= x) {
					module->startColorID = x-1;
				}
				
				*overrideRevert = true;
			}
		}
	};

	// random colour generation menu item
	struct RandomMenuItem : MenuItem {
		NVGcolor *color;
		int colorID;
		bool *overrideRevert;

		void onAction(const event::Action &e) override {

			// add a randomly coloured cable - must redo if it goes anywhere near black as the cables look horrible
			int r = 0, g = 0, b = 0;
			while(r < 32 && g < 32 && b < 32) {
				if (r < 32)
					r = (int)(random::uniform() * 255);
				
				if (g < 32)
					g = (int)(random::uniform() * 255);
				
				if (b < 32)
					b = (int)(random::uniform() * 255);
			}
			
			*color = nvgRGB(r, g, b);
			settings::cableColors[colorID] = *color;

			*overrideRevert = true;
		}
	};

	// color picker menu item
	struct ColorPickerMenuItem : MenuItem {
		NVGcolor rightColor;
		NVGcolor *color;
		int colorID;
		bool *enabled;
		bool *overrideRevert;

		void onAction(const event::Action &e) override {
			
			if (colorID < 0) {
				// add the colour
				*color = rightColor;
				settings::cableColors.push_back (*color);
				*enabled = true;
			}
			else {
				// set the cable colour
				*color = rightColor;
				settings::cableColors[colorID] = *color;
			}
			
			*overrideRevert = true;
		}

		void draw(const DrawArgs& args) override {
			BNDwidgetState state = BND_DEFAULT;

			if (APP->event->hoveredWidget == this)
				state = BND_HOVER;

			// Set active state if this MenuItem
			Menu *parentMenu = dynamic_cast<Menu*>(parent);
			if (parentMenu && parentMenu->activeEntry == this)
				state = BND_ACTIVE;

			if (active)
				state = BND_ACTIVE;

			// Main text and background
			if (!disabled)
				bndMenuItem(args.vg, 0.0, 0.0, box.size.x, box.size.y, state, -1, text.c_str());
			else
				bndMenuLabel(args.vg, 0.0, 0.0, box.size.x, box.size.y, -1, text.c_str());

			// Right text - use the colour we've specified
			float x = box.size.x - bndLabelWidth(args.vg, -1, rightText.c_str());
			bndIconLabelValue(args.vg, x, 0.0, box.size.x, box.size.y, -1, rightColor, BND_LEFT, BND_LABEL_FONT_SIZE, rightText.c_str(), NULL);
		}
	};

	// pick colour menu
	struct PickMenu : MenuItem {
		NVGcolor *color;
		int colorID;
		bool *overrideRevert;

		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			for (size_t i = 0; i < cableColors.size(); i ++) {				
				ColorPickerMenuItem *pickerItem = createMenuItem<ColorPickerMenuItem>(cableColorNames[i], "█");
			//	ColorPickerMenuItem *pickerItem = createMenuItem<ColorPickerMenuItem>(cableColorNames[i], "■");
				pickerItem->rightColor = cableColors[i];
				pickerItem->color = color;
				pickerItem->colorID = colorID;
				pickerItem->overrideRevert = overrideRevert;
				menu->addChild(pickerItem);
			}
			
			return menu;	
		}	
	};

	// add color menu
	struct AddMenu : MenuItem {
		NVGcolor *color;
		bool *enabled;
		bool *overrideRevert;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			for (size_t i = 0; i < cableColors.size(); i ++) {				
				ColorPickerMenuItem *pickerItem = createMenuItem<ColorPickerMenuItem>(cableColorNames[i], "█");
				//ColorPickerMenuItem *pickerItem = createMenuItem<ColorPickerMenuItem>(cableColorNames[i], "■");
				pickerItem->rightColor = cableColors[i];
				pickerItem->color = color;
				pickerItem->enabled = enabled;
				pickerItem->colorID = -1;
				pickerItem->overrideRevert = overrideRevert;
				menu->addChild(pickerItem);
			}
			
			return menu;	
		}	
	};

	//---------------------------------------------------------------------
	// Custom colour buttons
	//---------------------------------------------------------------------
	struct ColourButton : LightWidget {
		Palette *module;
		NVGcolor color;
		int colorID;
		bool enabled = true;
		bool overrideRevert;
		
		void createContextMenu() {
			ui::Menu *menu = createMenu();
			
			int x = (int)(settings::cableColors.size());

			MenuLabel *paramLabel = new MenuLabel;
			paramLabel->text = string::f("Cable Colour %d", colorID + 1);
			menu->addChild(paramLabel);
			
			overrideRevert = false;
			
			// for buttons already with colours, give the options to change or remove the colour
			if (colorID < x) {
				
				// colour picker for standard colours
				PickMenu *pickMenu = createMenuItem<PickMenu>("Pick", RIGHT_ARROW);
				pickMenu->color = &color;
				pickMenu->colorID = colorID;
				pickMenu->overrideRevert = &overrideRevert;
				menu->addChild(pickMenu);		
				
				// custom colour menu item
				ColorSliderMenu *sliderMenu =  createMenuItem<ColorSliderMenu>("Custom", RIGHT_ARROW);
				sliderMenu->color = &color;
				sliderMenu->originalColor = color;
				sliderMenu->colorID = colorID;
				sliderMenu->overrideRevert = &overrideRevert;
				menu->addChild(sliderMenu);		
				
				// option to generate a random colour
				RandomMenuItem *randomItem = createMenuItem<RandomMenuItem>("Random");
				randomItem->color = &color;
				randomItem->colorID = colorID;
				randomItem->overrideRevert = &overrideRevert;
				menu->addChild(randomItem);
				
				// only allow removal if this is not the only colour left
				if (x > 1) {
					MenuSeparator *ms = new (MenuSeparator);
					menu->addChild(ms);
						
					RemoveMenuItem *removeItem = createMenuItem<RemoveMenuItem>("Remove");
					removeItem->module = module;
					removeItem->colorID = colorID;
					removeItem->overrideRevert = &overrideRevert; // for stopping replacement of color if an adjustment has been made but not applied/cancelled
					menu->addChild(removeItem);
				}
			}

			// add function for first unassigned colour slot
			if (colorID == x) {
				AddMenu *addMenu = createMenuItem<AddMenu>("Add", RIGHT_ARROW);
				addMenu->color = &color;
				addMenu->enabled = &enabled;
				addMenu->overrideRevert = &overrideRevert;
				menu->addChild(addMenu);
			}
		}
		
		void draw(const DrawArgs& args) override {
			
			NVGcolor bezelColor = module ? module->bezelColor : SCHEME_BLACK;
			
			if (!enabled)
				bezelColor.a = 0.25;
			
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.0);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);
			
			nvgStrokeWidth(args.vg, 1.2);
			nvgStrokeColor(args.vg, bezelColor);
			nvgStroke(args.vg);
		}

		void onButton(const event::Button& e) override {
			Widget::onButton(e);
			e.stopPropagating();
			
			if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
				if (enabled) {
					APP->scene->rack->nextCableColorId = colorID;
					module->doChange = true;
				}
			}
			
			// Right click to open context menu
			if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT && (e.mods & RACK_MOD_MASK) == 0) {
				// only for buttons with colours associated or the first unallocated one
				if (colorID <= (int)(settings::cableColors.size()))
					createContextMenu();

				e.consume(this);
			}		
		}
	};

	struct CountModulaBezel : TransparentWidget {

		CountModulaBezel() {
			box.size = Vec(0, 0);	
		}
		
		void draw(const DrawArgs &args) override {
			
			// Background
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, box.size.x, box.size.y, 5.0); // 5mm for  3mm LED
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xA0));
			nvgFill(args.vg);
		}
	};


	std::string panelName;
	
	PaletteWidget(Palette *module) {
		setModule(module);
		bool moduleEnabled = (module ? module->running : true);
		panelName = moduleEnabled ? "Palette.svg" : "PaletteDisabled.svg";
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// add the buttons and lights for each of the first 7 available colours
		if (moduleEnabled) {
			if (!settings::cableColors.empty()) {
				
				int x = settings::cableColors.empty() ? 0: clamp(settings::cableColors.size(), 0, MAX_COLOURS);
				
				bool showAll = false;
				if (FORCE_MAX_COLOURS || x > NUM_COLOURS) {
					showAll = true;
					if (module)
						module->numColoursToUse = MAX_COLOURS;
				}
				
				NVGcolor color;
				bool enabled;
				
				int n = (module ? module->numColoursToUse : FORCE_MAX_COLOURS ? MAX_COLOURS : NUM_COLOURS);
				
				for (int i = 0; i < n; i++) {
					
					if (i < x && moduleEnabled) {
						color = settings::cableColors[i];
						enabled = true;
					}
					else {
						color = color::BLACK_TRANSPARENT;
						enabled = false;
					}
					
					ColourButton *colourButton = new ColourButton();
					if (showAll) {
						colourButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL1] - 25, STD_ROWS8[STD_ROW1] + (i * 19.0) - 15);
						colourButton->box.size = Vec(50, 15);
					}
					else {
						colourButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL1] - 25, STD_ROWS8[STD_ROW1 + i] - 15);
						colourButton->box.size = Vec(50, 30);
					}
					
					colourButton->module = module;
					colourButton->color = color;
					colourButton->colorID = i;
					colourButton->enabled = enabled;
					
					if (module)
						module->buttons.push_back(colourButton);
					
					addChild(colourButton);
				
					if (showAll) {
						addChild(createWidgetCentered<CountModulaBezel>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1]+ (i * 19.0) - 7.5)));
						addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1] + (i * 19.0) - 7.5), module, Palette::SELECT_LIGHTS + i));
					}
					else {
						addChild(createWidgetCentered<CountModulaBezel>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1 + i])));
						addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1 + i]), module, Palette::SELECT_LIGHTS + i));
					}
				}
			}
			
			// add the lock button
			addParam(createParamCentered<CountModulaLEDPushButtonNoRandom<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Palette::LOCK_PARAM, Palette::LOCK_PARAM_LIGHT));
		}
	}	
	
	// include the theme menu item struct we'll use when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Palette *module = dynamic_cast<Palette*>(this->module);
		assert(module);

		if (module->running) {
			// blank separator
			menu->addChild(new MenuSeparator());
			
			// add the theme menu items
			#include "../themes/themeMenus.hpp"
		}
	}
	
	const char *hotKeys = "123456789";
	
	void onHoverKey(const event::HoverKey &e) override {
		const char* key = glfwGetKeyName(e.key, 0);

		if (e.action == GLFW_PRESS && key && (e.mods & RACK_MOD_MASK) == (0x00)) {
			if (*key >= '1' && *key <= '9') {
				// colour selection hotkeys
				size_t i = *key - '1';			
				if (!settings::cableColors.empty() && i < settings::cableColors.size()) {
					
					APP->scene->rack->nextCableColorId = i;
					((Palette*)(module))->doChange = true;
				}
			}
			else if (*key == 'l') {
				// lock hotkey
				if (module->params[Palette::LOCK_PARAM].getValue() > 0.5)
					module->params[Palette::LOCK_PARAM].setValue(0.0);
				else
					module->params[Palette::LOCK_PARAM].setValue(1.0);
			}
		}
		ModuleWidget::onHoverKey(e);
	}
	

	void step() override {
		if (module) {
			Palette *m = (Palette *)module;
			if (m->running) {
				// process any change of theme
				#include "../themes/step.hpp"
				
				// process any cable colour removal
				if (m->colourRemoveID > -1) {
					
					int n = (m->buttons.size());
					
					int x = settings::cableColors.empty() ? 0: clamp(settings::cableColors.size(), 0, MAX_COLOURS);
					if (FORCE_MAX_COLOURS || x > NUM_COLOURS)
						m->numColoursToUse = MAX_COLOURS;
				
					for (int i = 0; i < n; i++) {
						
						ColourButton *b = (ColourButton *)(m->buttons[i]);
						if (i < x) {
							b->color = settings::cableColors[i];
							b->enabled = true;
						}
						else {
							b->color = color::BLACK_TRANSPARENT;
							b->enabled = false;
						}
					}
					
					m->colourRemoveID = -1;
				}
			}
		}
		
		Widget::step();
	}	
};

Model *modelPalette = createModel<Palette, PaletteWidget>("Palette");
