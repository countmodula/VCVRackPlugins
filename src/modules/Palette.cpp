//----------------------------------------------------------------------------
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

#define NUM_COLOURS 7

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
		ENUMS(SELECT_LIGHTS, NUM_COLOURS),
		NUM_LIGHTS
	};

	bool doChange;
	int startColorID = -1;
	int nextColorID = -1;
	int count = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Palette() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(LOCK_PARAM, 0.0f, 1.0f, 0.0f, "Lock current colour");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
	}	
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "color", json_integer(APP->scene->rack->nextCableColorId));	
		
		// add the theme details
		#include "../themes/dataToJson.hpp"				
		
		return root;	
	}

	void dataFromJson(json_t* root) override {
		
		json_t *col = json_object_get(root, "color");

		startColorID = -1;
		if (col)
			startColorID = json_integer_value(col);		
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}	
	
	void process(const ProcessArgs &args) override {

		int colorID = APP->scene->rack->nextCableColorId;
	
		// if we have locked the colour, continue with the colour we had chosen at the time the patch was saved 
		if (startColorID >= 0 && params[LOCK_PARAM].getValue() > 0.5f) {
			APP->scene->rack->nextCableColorId = colorID = startColorID;
			nextColorID = startColorID = -1;
			doChange = true;
		}
		
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
				for (int i = 0; i < NUM_COLOURS; i++) {
					lights[SELECT_LIGHTS + i].setBrightness(boolToLight(i == colorID));
				}
				
				// sync the next colour id
				nextColorID = colorID;
			}
		}
		
		if (++count > 8)
			count = 0;
		
		doChange = false;
	}
};

struct ColourButton : OpaqueWidget {
	Palette* module;
	NVGcolor color;
	int colorID;
	bool enabled = true;
	
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 1.0);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);
		
		nvgStrokeWidth(args.vg, 1.2);
		nvgStrokeColor(args.vg, module ? module->bezelColor : SCHEME_BLACK);
		nvgStroke(args.vg);
		
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (enabled) {
				APP->scene->rack->nextCableColorId = colorID;
				module->doChange = true;
			}
		}
		OpaqueWidget::onDragStart(e);
	}
};

struct CountModulaBezel : TransparentWidget {

	CountModulaBezel() {
		box.size = Vec(0, 0);	
	}
	
	void draw(const DrawArgs &args) override {
		
		// Background
		nvgBeginPath(args.vg);
		nvgCircle(args.vg, box.size.x, box.size.y, 7.0); // 7mm for  5mm LED
		nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xA0));
		nvgFill(args.vg);
	}
};

struct PaletteWidget : ModuleWidget {
	PaletteWidget(Palette *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Palette.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// add the buttons and lights for each of the first 6 available colours
		if (!settings::cableColors.empty()) {
			
			int x = settings::cableColors.empty() ? 0: clamp(settings::cableColors.size(), 0, NUM_COLOURS);

			NVGcolor color;
			bool enabled;
			for (int i = 0; i < NUM_COLOURS; i++) {
				
				if (i < x) {
					color = settings::cableColors[i];
					enabled = true;
				}
				else {
					color = color::BLACK_TRANSPARENT;
					enabled = false;
				}
				
				ColourButton* colourButton = new ColourButton();
				colourButton->box.pos = Vec(STD_COLUMN_POSITIONS[STD_COL1] - 25, STD_ROWS8[STD_ROW1 + i] - 15);
				colourButton->box.size = Vec(50, 30);
				colourButton->module = module;
				colourButton->color = color;
				colourButton->colorID = i;
				colourButton->enabled = enabled;
				addChild(colourButton);
				
				addChild(createWidgetCentered<CountModulaBezel>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1 + i])));
				
				addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 15, STD_ROWS8[STD_ROW1 + i]), module, Palette::SELECT_LIGHTS + i));
			}
		}
		
		// add the lock button
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Palette::LOCK_PARAM));
	}	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Palette *module = dynamic_cast<Palette*>(this->module);
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

Model *modelPalette = createModel<Palette, PaletteWidget>("Palette");
