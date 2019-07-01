//----------------------------------------------------------------------------
//	/^M^\ Count Modula - G2T Module
//	Gate To Trigger Module
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Inverter.hpp"

struct G2T : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		INV_OUTPUT,
		START_OUTPUT,
		END_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		GATE_LIGHT,
		START_LIGHT,
		END_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gate;
	dsp::PulseGenerator pgStart;
	dsp::PulseGenerator pgEnd;

	G2T() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}
	
	void onReset() override {
		gate.reset();
		pgStart.reset();
		pgEnd.reset();
	}
	
	void process(const ProcessArgs &args) override {
		
		// process the input
		gate.set(inputs[GATE_INPUT].getVoltage());

			// leading edge - fire the start trigger
		if (gate.leadingEdge())
			pgStart.trigger(1e-3f);

		// find leading/trailing edge
		if (gate.trailingEdge())
			pgEnd.trigger(1e-3f);
		
		// process the gate outputs
		if (gate.high()) {
			outputs[GATE_OUTPUT].setVoltage(10.0f); 
			outputs[INV_OUTPUT].setVoltage(0.0);
			lights[GATE_LIGHT].setBrightness(10.0f);
		}
		else {
			outputs[GATE_OUTPUT].setVoltage(0.0f); 
			outputs[INV_OUTPUT].setVoltage(10.0); 
			lights[GATE_LIGHT].setBrightness(0.0f);
		}
		
		// process the start trigger
		if (pgStart.process(args.sampleTime)) {
			outputs[START_OUTPUT].setVoltage(10.0f);
			lights[START_LIGHT].setSmoothBrightness(10.0f, args.sampleTime);
		}
		else {
			outputs[START_OUTPUT].setVoltage(0.0f);
			lights[START_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}

		// process the end trigger
		if (pgEnd.process(args.sampleTime)) {
			outputs[END_OUTPUT].setVoltage(10.0f);
			lights[END_LIGHT].setSmoothBrightness(10.0f, args.sampleTime);
		}
		else {
			outputs[END_OUTPUT].setVoltage(0.0f);
			lights[END_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
	}
};

struct G2TWidget : ModuleWidget {
G2TWidget(G2T *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/G2T.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1] + 12), module, G2T::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3]), module, G2T::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, G2T::INV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6] - 6), module, G2T::START_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8] - 12), module, G2T::END_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2] + 15), module, G2T::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5] + 6), module, G2T::START_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW7]), module, G2T::END_LIGHT));
	}
};

Model *modelG2T = createModel<G2T, G2TWidget>("G2T");
