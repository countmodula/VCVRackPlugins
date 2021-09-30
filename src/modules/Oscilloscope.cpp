//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Oscilloscope
//	Based on the VCV Scope by Andrew Belt.
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Oscilloscope
#define PANEL_FILE "Oscilloscope.svg"

#define BUFFER_SIZE 512

struct Oscilloscope : Module {
	enum ParamIds {
		CH1_SCALE_PARAM,
		CH2_SCALE_PARAM,
		CH3_SCALE_PARAM,
		CH4_SCALE_PARAM,
		CH1_POS_PARAM,
		CH2_POS_PARAM,
		CH3_POS_PARAM,
		CH4_POS_PARAM,
		CH1_ZERO_PARAM,
		CH2_ZERO_PARAM,
		CH3_ZERO_PARAM,
		CH4_ZERO_PARAM,
		TIME_PARAM,
		FREEZE_PARAM,
		TRIG_PARAM,
		SPARE_PARAM, // was the unused "one-shot" button.
		TRIGLEVEL_PARAM,
		HOLDOFF_PARAM,
		DISPLAY_GRID_PARAM,
		DISPLAY_GRIDBASELINE_PARAM,
		DISPLAY_TRACEBASELINE_PARAM,
		DISPLAY_STATISTICS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CH1_INPUT,
		CH2_INPUT,
		CH3_INPUT,
		CH4_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		CH1_ZERO_PARAM_LIGHT,
		CH2_ZERO_PARAM_LIGHT,
		CH3_ZERO_PARAM_LIGHT,
		CH4_ZERO_PARAM_LIGHT,
		FREEZE_PARAM_LIGHT,
		ENUMS(FREEZE_LIGHT,2),
		DISPLAY_GRID_PARAM_LIGHT,
		DISPLAY_GRIDBASELINE_PARAM_LIGHT,
		DISPLAY_TRACEBASELINE_PARAM_LIGHT,
		DISPLAY_STATISTICS_PARAM_LIGHT,
		NUM_LIGHTS
	};

	float buffer1[BUFFER_SIZE] = {};
	float buffer2[BUFFER_SIZE] = {};
	float buffer3[BUFFER_SIZE] = {};
	float buffer4[BUFFER_SIZE] = {};
	
	int bufferIndex = 0;
	float frameIndex = 0;

	dsp::SchmittTrigger sumTrigger;
	dsp::SchmittTrigger extTrigger;
	dsp::SchmittTrigger resetTrigger;
	
	bool freeze = false;
	bool freezeButton = false;
	
	// display options
	bool hideGrid = false;
	bool showGridBaseline = true;
	bool showTraceBaselines = false;
	bool showStats = false;
	int processCount = 16;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	

	Oscilloscope() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CH1_POS_PARAM, -10.0, 10.0, 0.0, "Ch 1 pos");
		configParam(CH2_POS_PARAM, -10.0, 10.0, 0.0, "Ch 2 pos");
		configParam(CH3_POS_PARAM, -10.0, 10.0, 0.0, "Ch 3 pos");
		configParam(CH4_POS_PARAM, -10.0, 10.0, 0.0, "Ch 4 pos");
		
		configParam(CH1_SCALE_PARAM, 0.0, 11.0, 2.0, "Ch 1 scale");
		configParam(CH2_SCALE_PARAM, 0.0, 11.0, 2.0, "Ch 2 scale");
		configParam(CH3_SCALE_PARAM, 0.0, 11.0, 2.0, "Ch 3 scale");
		configParam(CH4_SCALE_PARAM, 0.0, 11.0, 2.0, "Ch 4 scale");

		configParam(CH1_ZERO_PARAM, 0.0f, 1.0f, 0.0f, "Ch 1 zero");
		configParam(CH2_ZERO_PARAM, 0.0f, 1.0f, 0.0f, "Ch 2 zero");
		configParam(CH3_ZERO_PARAM, 0.0f, 1.0f, 0.0f, "Ch 3 zero");
		configParam(CH4_ZERO_PARAM, 0.0f, 1.0f, 0.0f, "Ch 4 zero");

		configParam(TRIG_PARAM, 0.0f, 4.0f, 1.0f, "Trigger source");
		configParam(TRIGLEVEL_PARAM, -10.0f, 10.0f, 0.0f, "Trigger level");
		configParam(HOLDOFF_PARAM, 0.0, 1000.0, 0.0, "Hold-off", "", 0, 0.01f);

		configParam(TIME_PARAM, -4.0, -18.0, -14.0, "Time");
		
		configParam(FREEZE_PARAM, 0.0f, 1.0f, 0.0f, "Trace freeze");
		configParam(DISPLAY_GRID_PARAM, 0.0f, 1.0f, 1.0f, "Show grid");
		configParam(DISPLAY_GRIDBASELINE_PARAM, 0.0f, 1.0f, 1.0f, "Show grid baseline");
		configParam(DISPLAY_TRACEBASELINE_PARAM, 0.0f, 1.0f, 0.0f, "show trace baselines");
		configParam(DISPLAY_STATISTICS_PARAM, 0.0f, 1.0f, 0.0f, "Show statistics");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		freeze = false;
		showGridBaseline = true;
		showTraceBaselines = false;
		hideGrid = false;
		showStats = false;
		
		processCount = 16;
	}
	
	void onReset() override {
		resetTrigger.reset();
		freeze = false;
		processCount = 16;
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
	
	void trigger() {
		resetTrigger.reset();
		bufferIndex = 0;
		frameIndex = 0;
	}
	
	void process(const ProcessArgs &args) override {

		// Compute times
		float deltaTime = powf(2.0, params[TIME_PARAM].getValue());
		int frameCount = (int)ceilf(deltaTime * args.sampleRate);

		float holdTime = params[HOLDOFF_PARAM].getValue() * deltaTime;

		// check if freeze is engaged
		if (params[FREEZE_PARAM].getValue() > 0.5f)
		{
			// toggle freeze indicator
			if (!freezeButton)
				freeze ^= true;
			
			freezeButton = true;
		}
		else
			freezeButton = false;

		if (++processCount > 16) {
			processCount = 0;
			showGridBaseline = params[DISPLAY_GRIDBASELINE_PARAM].getValue() > 0.5f;
			hideGrid = params[DISPLAY_GRID_PARAM].getValue() < 0.5f;
			showTraceBaselines = params[DISPLAY_TRACEBASELINE_PARAM].getValue() > 0.5f;
			showStats = params[DISPLAY_STATISTICS_PARAM].getValue() > 0.5f;
		}

		// Add frame to buffer
		if (bufferIndex < BUFFER_SIZE) {
			if (++frameIndex > frameCount) {
				frameIndex = 0;
				buffer1[bufferIndex] = inputs[CH1_INPUT].getVoltage();
				buffer2[bufferIndex] = inputs[CH2_INPUT].getVoltage();
				buffer3[bufferIndex] = inputs[CH3_INPUT].getVoltage();
				buffer4[bufferIndex] = inputs[CH4_INPUT].getVoltage();

				bufferIndex++;
			}
		}

		// Don't wait for trigger if still filling buffer
		if (bufferIndex < BUFFER_SIZE) {
			if (freeze) {
				lights[FREEZE_LIGHT].setBrightness(0.0f);
				lights[FREEZE_LIGHT+1].setBrightness(boolToLight(1.0f));
			}
			else
				lights[FREEZE_LIGHT+1].setBrightness(boolToLight(0.0f));
				
			return;
		}

		if (freeze) {
			lights[FREEZE_LIGHT].setBrightness(1.0f);
			lights[FREEZE_LIGHT+1].setBrightness(boolToLight(0.0f));
		}
		else {
			lights[FREEZE_LIGHT].setBrightness(0.0f);
			lights[FREEZE_LIGHT+1].setBrightness(boolToLight(0.0f));
		}

		// if freeze is engaged, we stop processing here so we no longer refresh the display
		if (freeze)
			return;

		bool connected = false, zero = false;
		float vTrig = 0.0f;
		switch ((int)(params[TRIG_PARAM].getValue())) {
			case 0:
				connected = inputs[TRIG_INPUT].isConnected();
				vTrig = inputs[TRIG_INPUT].getVoltage();
				break;
			case 1:
				connected = inputs[CH1_INPUT].isConnected();
				vTrig = inputs[CH1_INPUT].getVoltage();
				zero = params[CH1_ZERO_PARAM].getValue() > 0.5f;
				break;
			case 2:
				connected = inputs[CH2_INPUT].isConnected();
				vTrig = inputs[CH2_INPUT].getVoltage();
				zero = params[CH2_ZERO_PARAM].getValue() > 0.5f;
				break;
			case 3:
				connected = inputs[CH3_INPUT].isConnected();
				vTrig = inputs[CH3_INPUT].getVoltage();
				zero = params[CH3_ZERO_PARAM].getValue() > 0.5f;
				break;
			case 4:
				connected = inputs[CH4_INPUT].isConnected();
				vTrig = inputs[CH4_INPUT].getVoltage();
				zero = params[CH4_ZERO_PARAM].getValue() > 0.5f;
				break;
		}

		// Trigger immediately if nothing is plugged in to the selected trigger input or it is zeroed
		if (!connected || zero) {
			trigger();
			return;
		}

		frameIndex++;
	
		// Reset if triggered
		float trigThreshold = params[TRIGLEVEL_PARAM].getValue();
		if (resetTrigger.process(rescale(vTrig, trigThreshold, trigThreshold + 0.001f, 0.f, 1.f))) {
			trigger();
			return;
		}
		
		// Reset if we've been waiting for "holdTime"
		if (frameIndex * args.sampleTime >= holdTime) {
			trigger();
			return;
		}
	}
};

struct OscilloscopeDisplay : LightWidget {
	Oscilloscope *module;
	int frame = 0;
	std::shared_ptr<Font> font;

	const char* VoltsPerDiv[12] = {	"5V/Div",
									"2V/Div",
									"1V/Div",
									"500mV/Div",
									"200mV/Div",
									"100mV/Div",
									"50mV/Div",
									"20mV/Div",
									"10mV/Div",
									"5mV/Div",
									"2mV/Div",
									"1mV/Div"};

	const float GainFactors[12] = {	0.2f,	// 5V/Div
									0.5f,	// 2V/Div 
									1.0f,	// 1V/Div
									2.0f,	// 500mV/Div
									5.0f,	// 200mV/Div
									10.0f,	// 100mV/Div
									20.0f,	// 50mV/Div
									50.0f,	// 20mV/Div
									100.0f,	// 10mV/Div
									200.0f,	// 5mV/Div
									500.0f,	// 2mV/Div
									1000.0f	// 1mV/Div
									};

	struct Stats {
		float vpp = 0.f;
		float vmin = 0.f;
		float vmax = 0.f;
		void calculate(float *values) {
			vmax = -INFINITY;
			vmin = INFINITY;
			for (int i = 0; i < BUFFER_SIZE; i++) {
				float v = values[i];
				vmax = std::fmax(vmax, v);
				vmin = std::fmin(vmin, v);
			}
			vpp = vmax - vmin;
		}
	};
	
	Stats stats1, stats2, stats3, stats4;

	OscilloscopeDisplay() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Sudo.ttf"));
	}

	void drawWaveform(const DrawArgs &args, float *valuesX) {
		if (!valuesX)
			return;
		
		nvgSave(args.vg);
		Rect b = Rect(Vec(0, 0), box.size);
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
		
		nvgBeginPath(args.vg);
		// Draw maximum display left to right
		for (int i = 0; i < BUFFER_SIZE; i++) {
			float x, y;
			x = (float)i / (BUFFER_SIZE - 1);
			y = valuesX[i] / 2.0 + 0.5;

			Vec p;
			p.x = b.pos.x + b.size.x * x;
			p.y = b.pos.y + b.size.y * (1.0 - y);
			
			if (i == 0)
				nvgMoveTo(args.vg, p.x, p.y);
			else
				nvgLineTo(args.vg, p.x, p.y);
		}
		
		nvgLineCap(args.vg, NVG_ROUND);
		nvgMiterLimit(args.vg, 2.0);
		nvgStrokeWidth(args.vg, 1.5);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgStroke(args.vg);
		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}

	void drawTracemarker(const DrawArgs &args, int yPos) {
		
		nvgSave(args.vg);
		Rect b = Rect(Vec(0, 0), box.size);
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
		
		nvgBeginPath(args.vg);
		
		// Draw maximum display left to right
		for (int i = 0; i < BUFFER_SIZE; i++) {
			float x;
			//float y;
			x = (float)i / (BUFFER_SIZE - 1);
			//y = valuesX[i] / 2.0 + 0.5;

			Vec p;
			p.x = b.pos.x + b.size.x * x;
			//p.y = b.pos.y + b.size.y * (1.0 - y);
			
			if (i == yPos) {
				nvgMoveTo(args.vg, p.x, 0);
				nvgLineTo(args.vg, p.x, b.size.y);
			}
		}
		
		nvgLineCap(args.vg, NVG_ROUND);
		nvgMiterLimit(args.vg, 2.0);
		nvgStrokeWidth(args.vg, 1.5);
		nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
		nvgStroke(args.vg);
		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}	

	void drawStats(const DrawArgs& args, Vec pos, const char* title, Stats* stats, const char* scale) {
		nvgFontSize(args.vg, 13);
		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -1);

		nvgText(args.vg, pos.x + 4, pos.y + 11, title, NULL);

		pos = pos.plus(Vec(50, 11));

		std::string text;
		text = "PP:";
		text += isNear(stats->vpp, 0.f, 100.f) ? string::f("% 6.2f", stats->vpp) : "  ---";
		nvgText(args.vg, pos.x, pos.y, text.c_str(), NULL);
		text = "Max:";
		text += isNear(stats->vmax, 0.f, 100.f) ? string::f("% 6.2f", stats->vmax) : "  ---";
		nvgText(args.vg, pos.x + 70, pos.y, text.c_str(), NULL);
		text = "Min:";
		text += isNear(stats->vmin, 0.f, 100.f) ? string::f("% 6.2f", stats->vmin) : "  ---";
		nvgText(args.vg, pos.x + 145, pos.y, text.c_str(), NULL);
		
		nvgText(args.vg, pos.x + 220, pos.y, scale, NULL);
		
	}

	void drawBackground(const DrawArgs &args) override {

		nvgSave(args.vg);
		Rect b = Rect(Vec(0, 0), box.size);
		nvgScissor(args.vg, b.pos.x+1, b.pos.y+1, b.size.x-2, b.size.y-2);
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0.5);

		nvgFillColor(args.vg, nvgRGB(0x00, 0x00, 0x00));
		nvgFill(args.vg);

		nvgResetScissor(args.vg);
		nvgRestore(args.vg);
	}

	void drawBaseLine(const DrawArgs& args, float value) {
		Rect b = Rect(Vec(0, 0), box.size);
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);

		value = value / 2.f + 0.5f;
		Vec p = Vec(box.size.x, b.pos.y + b.size.y * (1.f - value));

		// Draw line
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, p.x-1, p.y);
		nvgLineTo(args.vg, 1, p.y);
		nvgClosePath(args.vg);
		nvgStroke(args.vg);
		nvgResetScissor(args.vg);
	}

	void draw (const DrawArgs &args) override {
		if(module == NULL) 
			return;

		// Disable tinting when rack brightness is decreased
		nvgGlobalTint(args.vg, color::WHITE);

		// hide the grid if we've chosen to do so
		if (module->hideGrid)
			drawBackground(args);
		
		// show the gid baseline if we've chosen to do so
		if (module->showGridBaseline) {
			nvgStrokeColor(args.vg, nvgRGBA(0xe6, 0xe6, 0xe6, 0xff));
			drawBaseLine(args, 0.0f);
		}

		// determine scales
		int scale1 = clamp((int)(module->params[Oscilloscope::CH1_SCALE_PARAM].getValue()), 0, 11);
		int scale2 = clamp((int)(module->params[Oscilloscope::CH2_SCALE_PARAM].getValue()), 0, 11);
		int scale3 = clamp((int)(module->params[Oscilloscope::CH3_SCALE_PARAM].getValue()), 0, 11);
		int scale4 = clamp((int)(module->params[Oscilloscope::CH4_SCALE_PARAM].getValue()), 0, 11);

		// determine offsets
		float offset1 = module->params[Oscilloscope::CH1_POS_PARAM].getValue() / 10.0f;
		float offset2 = module->params[Oscilloscope::CH2_POS_PARAM].getValue() / 10.0f;
		float offset3 = module->params[Oscilloscope::CH3_POS_PARAM].getValue() / 10.0f;
		float offset4 = module->params[Oscilloscope::CH4_POS_PARAM].getValue() / 10.0f;
		
		// which inputs are connected
		bool connected1 = module->inputs[Oscilloscope::CH1_INPUT].isConnected();
		bool connected2 = module->inputs[Oscilloscope::CH2_INPUT].isConnected();
		bool connected3 = module->inputs[Oscilloscope::CH3_INPUT].isConnected();
		bool connected4 = module->inputs[Oscilloscope::CH4_INPUT].isConnected();
		
		// zero button
		bool zero1 = module->params[Oscilloscope::CH1_ZERO_PARAM].getValue() > 0.5f;
		bool zero2 = module->params[Oscilloscope::CH2_ZERO_PARAM].getValue() > 0.5f;
		bool zero3 = module->params[Oscilloscope::CH3_ZERO_PARAM].getValue() > 0.5f;
		bool zero4 = module->params[Oscilloscope::CH4_ZERO_PARAM].getValue() > 0.5f;

		float values1[BUFFER_SIZE];
		float values2[BUFFER_SIZE];
		float values3[BUFFER_SIZE];
		float values4[BUFFER_SIZE];
		
		for (int i = 0; i < BUFFER_SIZE; i++) {
			int j = i;
			values1[i] = offset1 + (zero1 ? 0.0f : module->buffer1[j] * GainFactors[scale1] / 10.0);
			values2[i] = offset2 + (zero2 ? 0.0f : module->buffer2[j] * GainFactors[scale2] / 10.0);
			values3[i] = offset3 + (zero3 ? 0.0f : module->buffer3[j] * GainFactors[scale3] / 10.0);
			values4[i] = offset4 + (zero4 ? 0.0f : module->buffer4[j] * GainFactors[scale4] / 10.0);
		}

		// Calculate stats
		if (++frame >= 4) {
			frame = 0;
			stats1.calculate(module->buffer1);
			stats2.calculate(module->buffer2);
			stats3.calculate(module->buffer3);
			stats4.calculate(module->buffer4);
		}

		// draw the trace marker if we're running a low time base
		if (module->params[Oscilloscope::TIME_PARAM].getValue() > -10.0f) {
			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xc0));
			drawTracemarker(args, module->bufferIndex);
		}
		
		// Draw waveform 1 - Red
		if (connected1) {
			if (module->showTraceBaselines) {
				nvgStrokeColor(args.vg, nvgRGBA(0xff, 0x00, 0x00, 0x40));
				drawBaseLine(args, offset1);
			}
			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0x00, 0x00, 0xc0));
			drawWaveform(args, values1);
		}
				
		// Draw waveform 2 - Yellow
		if (connected2) {
			if (module->showTraceBaselines) {
				nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0x00, 0x40));
				drawBaseLine(args, offset2);
			}
			nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0x00, 0xc0));
			drawWaveform(args, values2);
		}
	
		// Draw waveform 3 - Green
		if (connected3) {
			if (module->showTraceBaselines) {
				nvgStrokeColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0x40));
				drawBaseLine(args, offset3);
			}
			nvgStrokeColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xc0));
			drawWaveform(args, values3);
		}
		
		// Draw waveform 4 - Blue 
		if (connected4) {
			if (module->showTraceBaselines) {
				nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x66, 0xff, 0x40));
				drawBaseLine(args, offset4);
			}
			nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x66, 0xff, 0xc0));
			drawWaveform(args, values4);
		}
		
		
		if (module->showStats) {
			int statsPos = 0;

			// draw stats for channel 1 if connected
			if (connected1) {
				nvgFillColor(args.vg, nvgRGBA(0xff, 0x00, 0x00, 0xff));
				drawStats(args, Vec(0, statsPos), "CH. 1", &stats1, VoltsPerDiv[scale1]);
				statsPos = statsPos + 15;
			}
			
			// draw stats for channel 4 if connected
			if (connected2) {
				nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0x00, 0xff));
				drawStats(args, Vec(0, statsPos), "Ch. 2", &stats2, VoltsPerDiv[scale2]);
				statsPos = statsPos + 15;
			}
			
			// draw stats for channel 4 if connected
			if (connected3) {
				nvgFillColor(args.vg, nvgRGBA(0x00, 0xff, 0x00, 0xff));
				drawStats(args, Vec(0, statsPos), "Ch. 3", &stats3, VoltsPerDiv[scale3]);
				statsPos = statsPos + 15;
			}

			// draw stats for channel 4 if connected
			if (connected4) {
				nvgFillColor(args.vg, nvgRGBA(0x00, 0x66, 0xff, 0xff));
				drawStats(args, Vec(0, statsPos), "Ch. 4", &stats4, VoltsPerDiv[scale4]);
			}
		}
	}
};

struct OscilloscopeWidget : ModuleWidget {
	std::string panelName;

	TransparentWidget *display;
	
	OscilloscopeWidget(Oscilloscope *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));
		
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	
		{
			OscilloscopeDisplay *display = new OscilloscopeDisplay();
			display->module = module;
			display->box.pos = Vec(240, 20);
			display->box.size = Vec(480, box.size.y - 20);
			addChild(display);
			this->display = display;
		}

		// channel 1 controls
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, Oscilloscope::CH1_INPUT));
		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Oscilloscope::CH1_SCALE_PARAM));
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, Oscilloscope::CH1_POS_PARAM));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]), module, Oscilloscope::CH1_ZERO_PARAM, Oscilloscope::CH1_ZERO_PARAM_LIGHT));
		
		// channel 2 controls
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, Oscilloscope::CH2_INPUT));
		addParam(createParamCentered<RotarySwitch<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Oscilloscope::CH2_SCALE_PARAM));
		addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, Oscilloscope::CH2_POS_PARAM));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<YellowLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW2]), module, Oscilloscope::CH2_ZERO_PARAM, Oscilloscope::CH2_ZERO_PARAM_LIGHT));

		// channel 3 controls
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Oscilloscope::CH3_INPUT));
		addParam(createParamCentered<RotarySwitch<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Oscilloscope::CH3_SCALE_PARAM));
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, Oscilloscope::CH3_POS_PARAM));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW3]), module, Oscilloscope::CH3_ZERO_PARAM, Oscilloscope::CH3_ZERO_PARAM_LIGHT));

		// channel 4 controls
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Oscilloscope::CH4_INPUT));
		addParam(createParamCentered<RotarySwitch<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Oscilloscope::CH4_SCALE_PARAM));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, Oscilloscope::CH4_POS_PARAM));
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<BlueLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW4]), module, Oscilloscope::CH4_ZERO_PARAM, Oscilloscope::CH4_ZERO_PARAM_LIGHT));
		
		// trigger section
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Oscilloscope::TRIG_INPUT));
		
		addParam(createParamCentered<RotarySwitch<OperatingAngle180<GreyKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Oscilloscope::TRIG_PARAM));
		addParam(createParamCentered<Potentiometer<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, Oscilloscope::TRIGLEVEL_PARAM));
		addParam(createParamCentered<Potentiometer<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW5]), module, Oscilloscope::HOLDOFF_PARAM));
		
		// timebase
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW6]), module, Oscilloscope::TIME_PARAM));
		
		// freeze function
		addParam(createParamCentered<CountModulaLEDPushButtonMiniMomentary<CountModulaPBLight<WhiteLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW6] + 8), module, Oscilloscope::FREEZE_PARAM, Oscilloscope::FREEZE_PARAM_LIGHT));
		addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW6] - 15), module, Oscilloscope::FREEZE_LIGHT));
		
		// display options
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<WhiteLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6] + 8), module, Oscilloscope::DISPLAY_GRID_PARAM, Oscilloscope::DISPLAY_GRID_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<WhiteLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW6] + 8), module, Oscilloscope::DISPLAY_GRIDBASELINE_PARAM, Oscilloscope::DISPLAY_GRIDBASELINE_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<WhiteLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6] + 8), module, Oscilloscope::DISPLAY_STATISTICS_PARAM, Oscilloscope::DISPLAY_STATISTICS_PARAM_LIGHT));
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<WhiteLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW6] + 8), module, Oscilloscope::DISPLAY_TRACEBASELINE_PARAM, Oscilloscope::DISPLAY_TRACEBASELINE_PARAM_LIGHT));
		
	}

	// include the theme menu item struct we'll use when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Oscilloscope *module = dynamic_cast<Oscilloscope*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"

	}
	
	void step() override{
		if (module) {
			display->box.size = Vec(480, box.size.y - 20);
			display->box.pos = Vec(240, 10);
			ModuleWidget::step();
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
	}
};

Model *modelOscilloscope = createModel<Oscilloscope, OscilloscopeWidget>("Oscilloscope");
