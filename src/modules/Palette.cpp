//---------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Cable Palette
//	Cable colour management tool
//	Hot key functionality borrowed from Stoermelder Stroke (C) Benjamin Dill
//  Copyright (C) 2021  Adam Verspaget
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

// Keynames (borrowed from Stoermelder Stroke).
static std::string keyName(int key) {

	switch (key) {
		case GLFW_KEY_SPACE:			return "SPACE";
		case GLFW_KEY_WORLD_1:			return "W1";
		case GLFW_KEY_WORLD_2:			return "W2";
		case GLFW_KEY_ESCAPE:			return "ESC";
		case GLFW_KEY_F1:				return "F1";
		case GLFW_KEY_F2:				return "F2";
		case GLFW_KEY_F3:				return "F3";
		case GLFW_KEY_F4:				return "F4";
		case GLFW_KEY_F5:				return "F5";
		case GLFW_KEY_F6:				return "F6";
		case GLFW_KEY_F7:				return "F7";
		case GLFW_KEY_F8:				return "F8";
		case GLFW_KEY_F9:				return "F9";
		case GLFW_KEY_F10:				return "F10";
		case GLFW_KEY_F11:				return "F11";
		case GLFW_KEY_F12:				return "F12";
		case GLFW_KEY_F13:				return "F13";
		case GLFW_KEY_F14:				return "F14";
		case GLFW_KEY_F15:				return "F15";
		case GLFW_KEY_F16:				return "F16";
		case GLFW_KEY_F17:				return "F17";
		case GLFW_KEY_F18:				return "F18";
		case GLFW_KEY_F19:				return "F19";
		case GLFW_KEY_F20:				return "F20";
		case GLFW_KEY_F21:				return "F21";
		case GLFW_KEY_F22:				return "F22";
		case GLFW_KEY_F23:				return "F23";
		case GLFW_KEY_F24:				return "F24";
		case GLFW_KEY_F25:				return "F25";
		case GLFW_KEY_UP:				return "UP";
		case GLFW_KEY_DOWN:				return "DOWN";
		case GLFW_KEY_LEFT:				return "LEFT";
		case GLFW_KEY_RIGHT:			return "RIGHT";
		case GLFW_KEY_TAB:				return "TAB";
		case GLFW_KEY_ENTER:			return "ENTER";
		case GLFW_KEY_BACKSPACE:		return "BS";
		case GLFW_KEY_INSERT:			return "INS";
		case GLFW_KEY_DELETE:			return "DEL";
		case GLFW_KEY_PAGE_UP:			return "PG-UP";
		case GLFW_KEY_PAGE_DOWN:		return "PG-DW";
		case GLFW_KEY_HOME:				return "HOME";
		case GLFW_KEY_END:				return "END";
		case GLFW_KEY_KP_0:				return "KEYPAD 0";
		case GLFW_KEY_KP_1:				return "KEYPAD 1";
		case GLFW_KEY_KP_2:				return "KEYPAD 2";
		case GLFW_KEY_KP_3:				return "KEYPAD 3";
		case GLFW_KEY_KP_4:				return "KEYPAD 4";
		case GLFW_KEY_KP_5:				return "KEYPAD 5";
		case GLFW_KEY_KP_6:				return "KEYPAD 6";
		case GLFW_KEY_KP_7:				return "KEYPAD 7";
		case GLFW_KEY_KP_8:				return "KEYPAD 8";
		case GLFW_KEY_KP_9:				return "KEYPAD 9";
		case GLFW_KEY_KP_DIVIDE:		return "KEYPAD /";
		case GLFW_KEY_KP_MULTIPLY:		return "KEYPAD *";
		case GLFW_KEY_KP_SUBTRACT:		return "KEYPAD -";
		case GLFW_KEY_KP_ADD:			return "KEYPAD +";
		case GLFW_KEY_KP_DECIMAL:		return "KEYPAD .";
		case GLFW_KEY_KP_EQUAL:			return "KEYPAD =";
		case GLFW_KEY_KP_ENTER:			return "KEYPAD ENTER";
		case GLFW_KEY_PRINT_SCREEN:		return "PRINT";
		case GLFW_KEY_PAUSE:			return "PAUSE";
		default:
			// moved here so we can differentiate between number pad and normal keyboard
			const char* k = glfwGetKeyName(key, 0);
			if (k) {
				std::string str = k;
				for (auto& c : str) c = std::toupper(c);
				return str;
			}		
	
			return "";
	}
}

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

#define NUM_SPECIAL_KEYS 19
static int SpecialKeys [NUM_SPECIAL_KEYS] = {
		GLFW_KEY_SEMICOLON,			/* ; */
		GLFW_KEY_EQUAL,				/* = */		
		GLFW_KEY_LEFT_BRACKET,		/* [ */
		GLFW_KEY_BACKSLASH,			/* \ */
		GLFW_KEY_RIGHT_BRACKET,		/* ] */
		GLFW_KEY_GRAVE_ACCENT,		/* ` */		
		GLFW_KEY_SPACE,             
		GLFW_KEY_APOSTROPHE,		/* ' */
		GLFW_KEY_COMMA,				/* , */
		GLFW_KEY_MINUS,				/* - */
		GLFW_KEY_PERIOD,			/* . */
		GLFW_KEY_SLASH,				/* / */	
		GLFW_KEY_KP_DECIMAL,
		GLFW_KEY_KP_DIVIDE,
		GLFW_KEY_KP_MULTIPLY,
		GLFW_KEY_KP_SUBTRACT,
		GLFW_KEY_KP_ADD,
		GLFW_KEY_KP_ENTER,
		GLFW_KEY_KP_EQUAL,
	};
	

#define NUM_NUMERIC_KEYS 20
static int NumericKeys [NUM_NUMERIC_KEYS] = {
		GLFW_KEY_0,
		GLFW_KEY_1,
		GLFW_KEY_2,
		GLFW_KEY_3,
		GLFW_KEY_4,
		GLFW_KEY_5,
		GLFW_KEY_6,
		GLFW_KEY_7,
		GLFW_KEY_8,
		GLFW_KEY_9,
		GLFW_KEY_KP_0,
		GLFW_KEY_KP_1,
		GLFW_KEY_KP_2,
		GLFW_KEY_KP_3,
		GLFW_KEY_KP_4,
		GLFW_KEY_KP_5,
		GLFW_KEY_KP_6,
		GLFW_KEY_KP_7,
		GLFW_KEY_KP_8,
		GLFW_KEY_KP_9	
	};	

#define NUM_ALPHA_KEYS 26
static int AlphaKeys[NUM_ALPHA_KEYS] = {
		GLFW_KEY_A,
		GLFW_KEY_B,
		GLFW_KEY_C,
		GLFW_KEY_D,
		GLFW_KEY_E,
		GLFW_KEY_F,
		GLFW_KEY_G,
		GLFW_KEY_H,
		GLFW_KEY_I,
		GLFW_KEY_J,
		GLFW_KEY_K,
		GLFW_KEY_L,
		GLFW_KEY_M,
		GLFW_KEY_N,
		GLFW_KEY_O,
		GLFW_KEY_P,
		GLFW_KEY_Q,
		GLFW_KEY_R,
		GLFW_KEY_S,
		GLFW_KEY_T,
		GLFW_KEY_U,
		GLFW_KEY_V,
		GLFW_KEY_W,
		GLFW_KEY_X,
		GLFW_KEY_Y,
		GLFW_KEY_Z	
	};

#define NUM_FUNCTION_KEYS 25
static int FunctionKeys[NUM_FUNCTION_KEYS] {
		GLFW_KEY_F1,
		GLFW_KEY_F2,
		GLFW_KEY_F3,
		GLFW_KEY_F4,
		GLFW_KEY_F5,
		GLFW_KEY_F6,
		GLFW_KEY_F7,
		GLFW_KEY_F8,
		GLFW_KEY_F9,
		GLFW_KEY_F10,
		GLFW_KEY_F11,
		GLFW_KEY_F12,
		GLFW_KEY_F13,
		GLFW_KEY_F14,
		GLFW_KEY_F15,
		GLFW_KEY_F16,
		GLFW_KEY_F17,
		GLFW_KEY_F18,
		GLFW_KEY_F19,
		GLFW_KEY_F20,
		GLFW_KEY_F21,
		GLFW_KEY_F22,
		GLFW_KEY_F23,
		GLFW_KEY_F24,
		GLFW_KEY_F25
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
		ENUMS(BUTTON_LIGHTS, MAX_COLOURS),
		NUM_LIGHTS
	};

	enum HotKeyAction {
		NO_HOTKEY_ACTION,
		LOCK_HOTKEY_ACTION,
		ENUMS(COLOUR_HOTKEY_ACTION, MAX_COLOURS),
		NUM_ACTIONS
	};
	
	bool doChange;
	int startColorID = -1;
	int nextColorID = -1;
	int count = 0;
	
	int numColoursToUse = FORCE_MAX_COLOURS ? MAX_COLOURS : NUM_COLOURS;
	int colourRemoveID = -1;
	
	std::vector<LightWidget *> buttons;
	
	// hotKeys
	int keyPressAction = NO_HOTKEY_ACTION;

	bool globalHotKeys =false;
	
	int hotKeyMapDefaults[MAX_COLOURS] = {
			GLFW_KEY_1,
			GLFW_KEY_2,
			GLFW_KEY_3,
			GLFW_KEY_4,
			GLFW_KEY_5,
			GLFW_KEY_6,
			GLFW_KEY_7,
			GLFW_KEY_8,
			GLFW_KEY_9,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN
		};
	
	int hotKeyMap[MAX_COLOURS] = {
			GLFW_KEY_1,
			GLFW_KEY_2,
			GLFW_KEY_3,
			GLFW_KEY_4,
			GLFW_KEY_5,
			GLFW_KEY_6,
			GLFW_KEY_7,
			GLFW_KEY_8,
			GLFW_KEY_9,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN,
			GLFW_KEY_UNKNOWN
		};
		
	int modifierMap[MAX_COLOURS] = {};
	int lockHotKey = GLFW_KEY_L;
	int lockModifier = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	bool running = false;
	
	Palette() {
		
		if (paletteSingleton == NULL) {
			paletteSingleton = this;
			running = true;
		}	
		
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configButton(LOCK_PARAM, "Lock current colour");

		getParamQuantity(LOCK_PARAM)->randomizeEnabled = false;

		for (int i = 0; i < MAX_COLOURS; i++) {
			configLight(BUTTON_LIGHTS + i, string::f("Select colour %d", i+1));
		}
		
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
	
	void readPaletteSettings() {
		// set the default values
		globalHotKeys = false;
		lockHotKey = GLFW_KEY_L;
		lockModifier = 0;
		for(int i = 0; i < MAX_COLOURS; i ++) {
			hotKeyMap[i] = hotKeyMapDefaults [i];
			modifierMap[i] = 0;
		}
		
		// read the settings file
		json_t *rootJ = readSettings();
		
		// grab the settings
		json_t* ghk = json_object_get(rootJ, "paletteGlobalHotKeys");
		json_t* lhk = json_object_get(rootJ, "paletteLockHotKey");
		json_t* lm = json_object_get(rootJ, "paletteLockModifier");
		json_t *hkm = json_object_get(rootJ, "paletteHotKeyMap");
		json_t *mm = json_object_get(rootJ, "paletteModifierMap");
		
		// now apply them if present.
		if (ghk)
			globalHotKeys = json_boolean_value(ghk);
			
		if (lhk)
			lockHotKey = json_integer_value(lhk);
		
		if (lm)
			lockModifier = json_integer_value(lm);

		for (int i = 0; i < MAX_COLOURS; i++) {
			if (hkm) {
				json_t *v = json_array_get(hkm, i);
				if (v)
					hotKeyMap[i] = json_integer_value(v);
			}
			
			if (mm) {
				json_t *v = json_array_get(mm, i);
				if (v)
					modifierMap[i] = json_integer_value(v);
			}
		}
		
		// houskeeping
		json_decref(rootJ);
	}
	
	void savePaletteSettings() {
		// read the existing settings from the file
		json_t *rootJ = readSettings();
		
		// add global hot key setting
		json_object_set_new(rootJ, "paletteGlobalHotKeys", json_boolean(globalHotKeys));

		// add lock key settings
		json_object_set_new(rootJ, "paletteLockHotKey", json_integer(lockHotKey));	
		json_object_set_new(rootJ, "paletteLockModifier", json_integer(lockModifier));	
		
		// no add the hot key and modifier maps or the colour buttons
		json_t *km = json_array();
		json_t *mm = json_array();
	
		for (int i = 0; i < MAX_COLOURS; i++) {
			json_array_insert_new(km, i, json_integer(hotKeyMap[i]));
			json_array_insert_new(mm, i, json_integer(modifierMap[i]));
		}		
		
		json_object_set_new(rootJ, "paletteHotKeyMap", km);
		json_object_set_new(rootJ, "paletteModifierMap", mm);	
		
		// save the updated data
		saveSettings(rootJ);
		
		// houskeeping
		json_decref(rootJ);
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

// keycapture widget
struct KeyContainer : Widget {
	Palette* module = NULL;

	void step() override {
		if (module && module->keyPressAction > Palette::NO_HOTKEY_ACTION) {
			if (module->keyPressAction == Palette::LOCK_HOTKEY_ACTION) {
				// colour lock hotkey
				if (module->params[Palette::LOCK_PARAM].getValue() > 0.5)
					module->params[Palette::LOCK_PARAM].setValue(0.0);
				else
					module->params[Palette::LOCK_PARAM].setValue(1.0);
			}
			else if (module->keyPressAction >= Palette::COLOUR_HOTKEY_ACTION) {
				// colour selection hotkeys
				size_t i = module->keyPressAction - Palette::COLOUR_HOTKEY_ACTION;
				if (!settings::cableColors.empty() && i < settings::cableColors.size()) {
					
					APP->scene->rack->nextCableColorId = i;
					module->doChange = true;
				}
			}

			module->keyPressAction = Palette::NO_HOTKEY_ACTION;
		}
		Widget::step();
	}

	void onHoverKey(const event::HoverKey& e) override {
		if (module && !module->isBypassed() && module->running && module->globalHotKeys) {
			if (e.action == GLFW_PRESS) {
				if (e.key == module->lockHotKey && ((e.mods & RACK_MOD_MASK) == module->lockModifier)) {
					// colour lock keypress
					module->keyPressAction = Palette::LOCK_HOTKEY_ACTION;
					e.consume(this);
				}
				else {
					// possible colour selection keypress
					for (size_t i = 0; i < NUM_COLOURS; i ++) {
						int k = module->hotKeyMap[i];
						
						if (k > -1 && e.key == k && ((e.mods & RACK_MOD_MASK) == module->modifierMap[i])) {		
							module->keyPressAction = Palette::COLOUR_HOTKEY_ACTION + i;
							e.consume(this);
							break;
						}
					}					
				}
			}
		}
		Widget::onHoverKey(e);
	}
};

struct PaletteWidget : ModuleWidget {
	KeyContainer* keyContainer = NULL;
	std::string panelName;
	
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
	// global hot key menu stuff
	//---------------------------------------------------------------------
	struct GlobalHotKeyMenuItem : MenuItem {
		Palette *module;
	
		void onAction(const event::Action& e) override {
			module->globalHotKeys ^= true;
			module->savePaletteSettings();
		}		
	};
	
	//---------------------------------------------------------------------
	// key selection menu stuff
	//---------------------------------------------------------------------

	// hotkey selection menu item
	struct HotKeySelectionMenuItem : MenuItem {
		Palette *module;
		int colorID;
		int key;
		
		void onAction(const event::Action& e) override {
			if (colorID < 0)
				module->lockHotKey = key;
			else
				module->hotKeyMap[colorID] = key;
				
			module->savePaletteSettings();
		}		
	};
	
	// hotkey number selection menu
	struct HotKeyNumberSelectionMenu : MenuItem {
		Palette *module;
		int colorID;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			int keyToUse = module->lockHotKey;
			if (colorID >= 0)
				keyToUse = module->hotKeyMap[colorID];
				
			for (int i = 0; i <  NUM_NUMERIC_KEYS; i ++) {			
				std::string s = keyName(NumericKeys[i]);
				if (s > "") {
					HotKeySelectionMenuItem *selectItem = createMenuItem<HotKeySelectionMenuItem>(s, CHECKMARK(NumericKeys[i] == keyToUse));
					selectItem->module = module;
					selectItem->key = NumericKeys[i];
					selectItem->colorID = colorID;
					menu->addChild(selectItem);
				}
			}

			return menu;	
		}			
	};
	
	// hotkey alpha selection menu
	struct HotKeyAlphaSelectionMenu : MenuItem {
		Palette *module;
		int colorID;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			int keyToUse = module->lockHotKey;
			if (colorID >= 0)
				keyToUse = module->hotKeyMap[colorID];
				
			for (int i = 0; i < NUM_ALPHA_KEYS; i ++) {			
				std::string s = keyName(AlphaKeys[i]);
				if (s > "") {
					HotKeySelectionMenuItem *selectItem = createMenuItem<HotKeySelectionMenuItem>(s, CHECKMARK(AlphaKeys[i] == keyToUse));
					selectItem->module = module;
					selectItem->key = AlphaKeys[i];
					selectItem->colorID = colorID;
					menu->addChild(selectItem);
				}
			}

			return menu;	
		}			
	};	
	
	// hotkey special character key selection menu
	struct HotKeySpecialSelectionMenu : MenuItem {
		Palette *module;
		int colorID;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			int keyToUse = module->lockHotKey;
			if (colorID >= 0)
				keyToUse = module->hotKeyMap[colorID];			
			
			for (int i = 0; i < NUM_SPECIAL_KEYS; i ++) {			
				std::string s = keyName(SpecialKeys[i]);
				if (s > "") {
					HotKeySelectionMenuItem *selectItem = createMenuItem<HotKeySelectionMenuItem>(s, CHECKMARK(SpecialKeys[i] == keyToUse));
					selectItem->module = module;
					selectItem->key = SpecialKeys[i];
					selectItem->colorID = colorID;
					menu->addChild(selectItem);
				}
			}

			return menu;	
		}			
	};		
	
	// hotkey function key selection menu
	struct HotKeyFunctionSelectionMenu : MenuItem {
		Palette *module;
		int colorID;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			
			int keyToUse = module->lockHotKey;
			if (colorID >= 0)
				keyToUse = module->hotKeyMap[colorID];
			
			for (int i = 0; i < NUM_FUNCTION_KEYS; i ++) {			
				std::string s = keyName(FunctionKeys[i]);
				if (s > "") {
					HotKeySelectionMenuItem *selectItem = createMenuItem<HotKeySelectionMenuItem>(s, CHECKMARK(FunctionKeys[i] == keyToUse));
					selectItem->module = module;
					selectItem->key = FunctionKeys[i];
					selectItem->colorID = colorID;
					menu->addChild(selectItem);
				}
			}

			return menu;	
		}			
	};
	
	struct HotKeyModifierMenuItem : MenuItem {
		Palette *module;
		int colorID;
		int modifier;
		
		void onAction(const event::Action& e) override {
			
			if (colorID < 0) {
				// we're doing the lock modifier
				if ((module->lockModifier & modifier) == modifier)
					module->lockModifier &= ~modifier;
				else
					module->lockModifier |= modifier;
			}
			else {
				if ((module->modifierMap[colorID] & modifier) == modifier)
					module->modifierMap[colorID] &= ~modifier;
				else
					module->modifierMap[colorID] |= modifier;
			}
			
			module->savePaletteSettings();
		}		
	};
	
	// hotkey selection menu
	struct HotKeySelectionMenu : MenuItem {
		Palette *module;
		int colorID;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			menu->addChild(createMenuLabel("Key:"));
			
			// lock or colour hot key?
			int keyToUse = module->lockHotKey;
			int modifierToUse = module->lockModifier;
			if (colorID >= 0) {
				keyToUse = module->hotKeyMap[colorID];
				modifierToUse = module->modifierMap[colorID];
			}
			
			int *ak = std::find(AlphaKeys, AlphaKeys + NUM_ALPHA_KEYS, keyToUse);
			HotKeyAlphaSelectionMenu *alphaItem = createMenuItem<HotKeyAlphaSelectionMenu>("Alpha", (ak != AlphaKeys + NUM_ALPHA_KEYS) ? CHECKMARK_STRING : RIGHT_ARROW);
			alphaItem->module = module;
			alphaItem->colorID = colorID;
			menu->addChild(alphaItem);			
			
			int *nk = std::find(NumericKeys, NumericKeys + NUM_NUMERIC_KEYS, keyToUse);
			HotKeyNumberSelectionMenu *numItem = createMenuItem<HotKeyNumberSelectionMenu>("Numeric", (nk != NumericKeys + NUM_NUMERIC_KEYS) ? CHECKMARK_STRING : RIGHT_ARROW);
			numItem->module = module;
			numItem->colorID = colorID;
			menu->addChild(numItem);

			int *sk = std::find(SpecialKeys, SpecialKeys + NUM_SPECIAL_KEYS, keyToUse);
			HotKeySpecialSelectionMenu *specItem = createMenuItem<HotKeySpecialSelectionMenu>("Special", (sk != SpecialKeys + NUM_SPECIAL_KEYS) ? CHECKMARK_STRING : RIGHT_ARROW);
			specItem->module = module;
			specItem->colorID = colorID;
			menu->addChild(specItem);
			
			int *fk = std::find(FunctionKeys, FunctionKeys + NUM_FUNCTION_KEYS, keyToUse);
			HotKeyFunctionSelectionMenu *funcItem = createMenuItem<HotKeyFunctionSelectionMenu>("Function", (fk != FunctionKeys + NUM_FUNCTION_KEYS) ? CHECKMARK_STRING : RIGHT_ARROW);
			funcItem->module = module;
			funcItem->colorID = colorID;
			menu->addChild(funcItem);
			
			MenuSeparator *ms = new (MenuSeparator);
			menu->addChild(ms);			
			menu->addChild(createMenuLabel("Modifiers:"));
			
			HotKeyModifierMenuItem *ctrlItem = createMenuItem<HotKeyModifierMenuItem>(RACK_MOD_CTRL_NAME, CHECKMARK((modifierToUse & RACK_MOD_CTRL) == RACK_MOD_CTRL));
			ctrlItem->module = module;
			ctrlItem->colorID = colorID;
			ctrlItem->modifier = RACK_MOD_CTRL;
			menu->addChild(ctrlItem);
		
			HotKeyModifierMenuItem *shftItem = createMenuItem<HotKeyModifierMenuItem>(RACK_MOD_ALT_NAME, CHECKMARK((modifierToUse & GLFW_MOD_ALT) == GLFW_MOD_ALT));
			shftItem->module = module;
			shftItem->colorID = colorID;
			shftItem->modifier = GLFW_MOD_ALT;
			menu->addChild(shftItem);		
			
			HotKeyModifierMenuItem *altItem = createMenuItem<HotKeyModifierMenuItem>(RACK_MOD_SHIFT_NAME, CHECKMARK((modifierToUse & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT));
			altItem->module = module;
			altItem->colorID = colorID;
			altItem->modifier = GLFW_MOD_SHIFT;
			menu->addChild(altItem);		
		
			return menu;
		}			
	};	


	// custom tool tip for the colour buttons
	struct ColourButtonTooltip : ui::Tooltip {
		ModuleLightWidget* lightWidget;

		// hack to make the mose-overs look like button tooltips.
		void step() override {
			if (lightWidget->module) {
				engine::LightInfo* lightInfo = lightWidget->getLightInfo();
				if (!lightInfo)
					return;
				// Label
				text = lightInfo->getName();
				
				// Description
				std::string description = lightInfo->getDescription();
				if (description != "") {
					text += description;
				}

			}
			Tooltip::step();
			// Position at bottom-right of parameter
			box.pos = lightWidget->getAbsoluteOffset(lightWidget->box.size).round();
			// Fit inside parent (copied from Tooltip.cpp)
			assert(parent);
			box = box.nudge(parent->box.zeroPos());
		}
	};


	//---------------------------------------------------------------------
	// Custom colour buttons
	//---------------------------------------------------------------------
	struct ColourButton : ModuleLightWidget {
		NVGcolor color;
		int colorID;
		bool enabled = true;
		bool overrideRevert;
		
		void createContextMenu() {
			Palette *module = dynamic_cast<Palette*>(this->module);
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
			
			// add hotkey selection menu stuff
			MenuSeparator *ms = new (MenuSeparator);
			menu->addChild(ms);
			
			// display current hot key so users can see it without drilling down
			std::string s = keyName(module->hotKeyMap[colorID]);
			std::string pl = " + ";
			if (s > "") {
				if ((module->modifierMap[colorID] & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT)
					s = RACK_MOD_SHIFT_NAME + pl + s;
				
				if ((module->modifierMap[colorID] & GLFW_MOD_ALT) == GLFW_MOD_ALT)
					s = RACK_MOD_ALT_NAME + pl + s;
				
				if ((module->modifierMap[colorID] & RACK_MOD_CTRL) == RACK_MOD_CTRL)
					s = RACK_MOD_CTRL_NAME + pl + s;
					
				s = "Hotkey:  " + s;
			}
			else 
				s = "Hotkey:";
				
			HotKeySelectionMenu *hksMenuItem = createMenuItem<HotKeySelectionMenu>(s, RIGHT_ARROW);
			hksMenuItem->module = module;
			hksMenuItem->colorID = colorID;
			menu->addChild(hksMenuItem);
		}
		
		void draw(const DrawArgs& args) override {
			
			NVGcolor bezelColor = module ? ((Palette *)(module))->bezelColor : SCHEME_BLACK;
			
			nvgGlobalTint(args.vg, color::WHITE);
			
			if (!enabled)
				bezelColor.a = 0.25;
			
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.0);
			nvgFillColor(args.vg, color);
			nvgFill(args.vg);
			
			nvgStrokeWidth(args.vg, 1.2);
			nvgStrokeColor(args.vg, bezelColor);
			nvgStroke(args.vg);
			
			ModuleLightWidget::draw(args);
		}

		void drawHalo(const DrawArgs& args) override {
			// Don't draw halo if rendering in a framebuffer, e.g. screenshots or Module Browser
			if (args.fb)
				return;

			const float halo = settings::haloBrightness;
			if (halo == 0.f)
				return;

			// If light is off, rendering the halo gives no effect.
			if (this->color.r == 0.f && this->color.g == 0.f && this->color.b == 0.f)
				return;

			float br = 30.0; // Blur radius
			float cr = 5.0; // Corner radius
			
			nvgBeginPath(args.vg);
			nvgRect(args.vg, -br, -br, this->box.size.x + 2 * br, this->box.size.y + 2 * br);
			NVGcolor icol = color::mult(color, halo);
			NVGcolor ocol = nvgRGBA(0, 0, 0, 0);
			nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, this->box.size.x, this->box.size.y, cr, br, icol, ocol));
			nvgFill(args.vg);
		}

		void onButton(const event::Button& e) override {
			Palette *module = dynamic_cast<Palette*>(this->module);
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
		
		// hack to customise the tool tips
		void createTooltip() {
			if (!settings::tooltips)
				return;
			if (this->tooltip)
				return;
			// If the LightInfo is null, don't show a tooltip
			if (!getLightInfo())
				return;
			ColourButtonTooltip* tooltip = new ColourButtonTooltip;
			tooltip->lightWidget = this;
			APP->scene->addChild(tooltip);
			this->tooltip = tooltip;
		}
		
		// hack to customise the tooltips
		void onEnter(const EnterEvent& e) override {
			this->createTooltip();
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

	// hotkey  menu
	struct HotKeyMenu : MenuItem {
		Palette *module;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// global hotkey menu item
			GlobalHotKeyMenuItem *ghkMenuItem = createMenuItem<GlobalHotKeyMenuItem>("Global", CHECKMARK(module->globalHotKeys));
			ghkMenuItem->module = module;
			menu->addChild(ghkMenuItem);
	
			// display current hot key so users can see it without drilling down
			std::string s = keyName(module->lockHotKey);
			std::string pl = " + ";
			if (s > "") {
				if ((module->lockModifier & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT)
					s = RACK_MOD_SHIFT_NAME + pl + s;
				
				if ((module->lockModifier & GLFW_MOD_ALT) == GLFW_MOD_ALT)
					s = RACK_MOD_ALT_NAME + pl + s;
				
				if ((module->lockModifier & RACK_MOD_CTRL) == RACK_MOD_CTRL)
					s = RACK_MOD_CTRL_NAME + pl + s;
					
				s = "Lock Hotkey:  " + s;
			}
			else 
				s = "Lock Hotkey:";
				
			// lock hot key menu
			HotKeySelectionMenu *hksMenuItem = createMenuItem<HotKeySelectionMenu>(s, RIGHT_ARROW);
			hksMenuItem->module = module;
			hksMenuItem->colorID = -1; // -1 indicates lock hot key
			menu->addChild(hksMenuItem);		
		
			return menu;
		}			
	};	

	//---------------------------------------------------------------------
	// palette widget
	//---------------------------------------------------------------------
	PaletteWidget(Palette *module) {
		setModule(module);
		bool moduleEnabled = (module ? module->running : true);
		panelName = moduleEnabled ? "Palette.svg" : "PaletteDisabled.svg";
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		if (module && moduleEnabled) {
			module->readPaletteSettings();
			keyContainer = new KeyContainer;
			keyContainer->module = module;
			// This is where the magic happens: add a new widget on top-level to Rack
			APP->scene->rack->addChild(keyContainer);
		}

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
				
				int lightId = Palette::BUTTON_LIGHTS;
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
					colourButton->firstLightId = lightId++;

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
			addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Palette::LOCK_PARAM, Palette::LOCK_PARAM_LIGHT));
			
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
			
			HotKeyMenu *hkMenu = createMenuItem<HotKeyMenu>("Hotkeys", RIGHT_ARROW);
			hkMenu->module = module;
			menu->addChild(hkMenu);
		}
	}
	
	void onHoverKey(const event::HoverKey &e) override {
		if (!((Palette*)(module))->globalHotKeys) {
			if (e.action == GLFW_PRESS ) {
				if (e.key == ((Palette*)(module))->lockHotKey && ((e.mods & RACK_MOD_MASK) == ((Palette*)(module))->lockModifier)) {
					// lock key press
					if (module->params[Palette::LOCK_PARAM].getValue() > 0.5)
						module->params[Palette::LOCK_PARAM].setValue(0.0);
					else
						module->params[Palette::LOCK_PARAM].setValue(1.0);
						
					e.consume(this);
				}
				else {
					// possible colour selection key press
					for (size_t i = 0; i < NUM_COLOURS; i ++) {
						int k = ((Palette*)(module))->hotKeyMap[i];
						
						if (k > -1 && e.key == k && ((e.mods & RACK_MOD_MASK) == ((Palette*)(module))->modifierMap[i])) {		
							if (!settings::cableColors.empty() && i < settings::cableColors.size()) {
								
								APP->scene->rack->nextCableColorId = i;
								((Palette*)(module))->doChange = true;
							}
							
							e.consume(this);
							break;
						}
					}
				}
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
	
	~PaletteWidget() {
		if (keyContainer) {
			APP->scene->rack->removeChild(keyContainer);
			delete keyContainer;
		}
	}
};

Model *modelPalette = createModel<Palette, PaletteWidget>("Palette");
