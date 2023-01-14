//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack -Stack
//	A FIFO/LIFO voltage stack
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include <queue>
#include <stack>

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Stack
#define PANEL_FILE "Stack.svg"

#define MAX_ELEMENTS 16

struct StackData {
	float cvA;
	float cvB;
	float cvC;
	float cvD;
};

struct Stack : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	
	enum InputIds {
		POP_INPUT,
		PUSH_INPUT,
		RESET_INPUT,
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		NUM_INPUTS
	};
	
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		OVERFLOW_OUTPUT,
		UNDERFLOW_OUTPUT,
		FULL_OUTPUT,
		EMPTY_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		OVERFLOW_LIGHT,
		FULL_LIGHT,
		EMPTY_LIGHT,
		UNDERFLOW_LIGHT,
		FIFO_LIGHT,
		LIFO_LIGHT,
		ENUMS(POINTER_LIGHTS, MAX_ELEMENTS),
		NUM_LIGHTS
	};
	
	
	enum Modes {
		FIFO,
		LIFO
	};
	
	GateProcessor gpPush;
	GateProcessor gpPop;
	GateProcessor gpReset;

	std::queue<StackData> fifoQueue;
	std::stack<StackData> lifoStack;
	
	int mode = FIFO;
	int prevMode = -1;
	float cvA = 0.0f, cvB = 0.0f, cvC = 0.0f, cvD = 0.0f;
	float overflow = 0.0f;
	float underflow = 0.0f;
	float empty = 10.0f;
	float full = 0.0f;
	int stackSize = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Stack() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configInput(RESET_INPUT, "Reset");
		configInput(PUSH_INPUT, "Push");
		configInput(POP_INPUT, "Pop");
		
		configInput(A_INPUT, "A");
		configInput(B_INPUT, "B");
		configInput(C_INPUT, "C");
		configInput(D_INPUT, "D");
		
		configOutput(OVERFLOW_OUTPUT, "Overflow");
		configOutput(FULL_OUTPUT, "Full");
		configOutput(EMPTY_OUTPUT, "Empty");
		configOutput(UNDERFLOW_OUTPUT, "Underflow");

		configOutput(A_OUTPUT, "A");
		configOutput(B_OUTPUT, "B");
		configOutput(C_OUTPUT, "C");
		configOutput(D_OUTPUT, "D");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		onReset();
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		json_object_set_new(root, "mode", json_integer(mode));

		// add the theme details
		#include "../themes/dataToJson.hpp"		
		return root;
	}

	void dataFromJson(json_t* root) override {

		json_t *m = json_object_get(root, "mode");

		mode = FIFO;
		prevMode = -1;
		if (m) {
			if (json_integer_value(m) == LIFO) {
				mode = LIFO;
			}
		}

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void onReset() override {
		
		clearStack();	
		
		gpPush.reset();
		gpPop.reset();
		gpReset.reset();
		
		cvA = cvB = cvC = cvD = 0.0f;
		
		prevMode = -1;
		mode = FIFO;
	}
	
	void clearStack() {	
		while(!fifoQueue.empty()) {
			fifoQueue.pop();
		}
		
		while(!lifoStack.empty()) {
			lifoStack.pop();
		}
		
		full = overflow = underflow = 0.0f;
		empty = 10.0f;
		
		stackSize = 0;
	}
	
	void process(const ProcessArgs &args) override {

		bool doLights = false;
		
		if (mode != prevMode) {
			clearStack();
			
			lights[OVERFLOW_LIGHT].setBrightness(0.0f);
			lights[UNDERFLOW_LIGHT].setBrightness(0.0f);			
			
			switch (mode) {
				case FIFO:
					lights[FIFO_LIGHT].setBrightness(1.0f);
					lights[LIFO_LIGHT].setBrightness(0.0f);
					break;
				case LIFO:
					lights[FIFO_LIGHT].setBrightness(0.0f);
					lights[LIFO_LIGHT].setBrightness(1.0f);
					break;
			}
			
			prevMode = mode;
			
			doLights = true;
		}		
		
		// process the inputs
		gpReset.set(inputs[RESET_INPUT].getVoltage());
		gpPush.set(inputs[PUSH_INPUT].getVoltage());
		gpPop.set(inputs[POP_INPUT].getVoltage());
		
		
		// leading edge initiates the reset
		if (gpReset.leadingEdge()) {
			clearStack();
			
			underflow = overflow = 0.0f;
			doLights = true;
		}
		else if (gpReset.low()) {
			// high reset level inhibits the stack.
			
			if (gpPush.leadingEdge()) {
				
				// can grab these here
				StackData x;
				x.cvA = inputs[A_INPUT].getVoltage();
				x.cvB = inputs[B_INPUT].getVoltage();
				x.cvC = inputs[C_INPUT].getVoltage();
				x.cvD = inputs[D_INPUT].getVoltage();

				underflow = 0.0f; // cannot be under if we're pushing onto the stack
				
				switch (mode) {
					case FIFO:
						if (fifoQueue.size() >= MAX_ELEMENTS) {
							// overflow
							overflow = 10.0f;
						}
						else {
							fifoQueue.push(x);
							overflow = 0.0f;
						}

						stackSize = fifoQueue.size();
	
						break;
					case LIFO:
						if (lifoStack.size() >= MAX_ELEMENTS) {
							// overflow
							overflow = 10.0f;
						}
						else {
							lifoStack.push(x);
							overflow = 0.0f;
						}
						
						stackSize = lifoStack.size();
						
						break;
				}
				
				doLights = true;
			}
			
			if (gpPop.leadingEdge()) {
				overflow = 0.0f; // cannot be over if we're popping off the stack
				
				switch(mode) {
					case FIFO:
						if (fifoQueue.empty()) {
							// underflow
							cvA = cvB = cvC = cvD = 0.0f;
							underflow = 10.0f;
						}
						else {
							StackData x = fifoQueue.front();
							
							fifoQueue.pop();
							
							cvA = x.cvA;
							cvB = x.cvB;
							cvC = x.cvC;
							cvD = x.cvD;
							
							underflow = 0.0f;
						}						
				
						stackSize = fifoQueue.size();
						
						break;
					case LIFO:
						if (lifoStack.empty()) {

							// underflow
							cvA = cvB = cvC = cvD = 0.0f;
							underflow = 10.0f;
						}
						else {
							StackData x = lifoStack.top();
							lifoStack.pop();
							
							cvA = x.cvA;
							cvB = x.cvB;
							cvC = x.cvC;
							cvD = x.cvD;

							underflow = 0.0f;
						}		
						
						stackSize = lifoStack.size();

						break;
				}
				
				doLights = true;
			}
			
			empty = boolToGate(stackSize == 0);
			full = boolToGate(stackSize == MAX_ELEMENTS);		
		}

		if (doLights) {
			lights[OVERFLOW_LIGHT].setBrightness(overflow/10.0f);
			lights[FULL_LIGHT].setBrightness(full/10.0f);
			lights[EMPTY_LIGHT].setBrightness(empty/10.0f);
			lights[UNDERFLOW_LIGHT].setBrightness(underflow/10.0f);

			int led = POINTER_LIGHTS;
			for (int l = 0; l < MAX_ELEMENTS; l++) {
				if (l < stackSize) {
					lights[led++].setBrightness(1.0f);
				}
				else {
					lights[led++].setBrightness(0.0f);
				}
			}
			
		}

		outputs[OVERFLOW_OUTPUT].setVoltage(overflow);
		outputs[FULL_OUTPUT].setVoltage(full);
		outputs[EMPTY_OUTPUT].setVoltage(empty);
		outputs[UNDERFLOW_OUTPUT].setVoltage(underflow);
		
		outputs[A_OUTPUT].setVoltage(cvA);
		outputs[B_OUTPUT].setVoltage(cvB);
		outputs[C_OUTPUT].setVoltage(cvC);
		outputs[D_OUTPUT].setVoltage(cvD);
	}
};

struct StackWidget : ModuleWidget {
	std::string panelName;
	
	StackWidget(Stack *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"
		
		// screws
		#include "../components/stdScrews.hpp"

		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, Stack::RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, Stack::PUSH_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, Stack::POP_INPUT));

		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, Stack::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, Stack::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, Stack::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, Stack::D_INPUT));

		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1]), module, Stack::OVERFLOW_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW2]), module, Stack::FULL_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW3]), module, Stack::EMPTY_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW4]), module, Stack::UNDERFLOW_OUTPUT));
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW5]), module, Stack::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW6]), module, Stack::B_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW7]), module, Stack::C_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW8]), module, Stack::D_OUTPUT));
		
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW1] - 19), module, Stack::OVERFLOW_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW2] - 19), module, Stack::FULL_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW3] - 19), module, Stack::EMPTY_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW4] - 19), module, Stack::UNDERFLOW_LIGHT));
		
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 9, STD_ROWS8[STD_ROW1] - 6), module, Stack::FIFO_LIGHT));
		addChild(createLightCentered<SmallLight<WhiteLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 9, STD_ROWS8[STD_ROW1] + 6), module, Stack::LIFO_LIGHT));
		
		// stack status leds - rendered bottom to top.
		float row = STD_ROWS8[STD_ROW8];
		float col = STD_COLUMN_POSITIONS[STD_COL2] - 4;
		for (int l = 0; l < MAX_ELEMENTS; l++) {
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(col, row), module, Stack::POINTER_LIGHTS + l));
			row -= 19.6f;
		}
	}
	
	struct ModeMenuItem : MenuItem {
		Stack *module;
		int newMode = 0;
		
		void onAction(const event::Action &e) override {
			module->mode = newMode;
		}
	};
	
	struct ModeMenu : MenuItem {
		Stack *module;
		
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			ModeMenuItem *fifoMenuItem = createMenuItem<ModeMenuItem>("First In First Out (FIFO)", CHECKMARK(module->mode == Stack::FIFO));
			fifoMenuItem->module = module;
			fifoMenuItem->newMode = Stack::FIFO;
			menu->addChild(fifoMenuItem);

			ModeMenuItem *lifoMenuItem = createMenuItem<ModeMenuItem>("Last In First Out (LIFO)", CHECKMARK(module->mode == Stack::LIFO));
			lifoMenuItem->module = module;
			lifoMenuItem->newMode = Stack::LIFO;
			menu->addChild(lifoMenuItem);

			return menu;
		}
	};
	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Stack *module = dynamic_cast<Stack*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// stack mode
		ModeMenu *modeMenuItem = createMenuItem<ModeMenu>("Mode", RIGHT_ARROW);
		modeMenuItem->module = module;
		menu->addChild(modeMenuItem);
	}	
	
	void step() override {
		if (module) {
			// Stack *m = dynamic_cast<Stack*>(this->module);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}
};

Model *modelStack = createModel<Stack, StackWidget>("Stack");
