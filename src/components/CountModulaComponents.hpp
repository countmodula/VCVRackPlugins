//----------------------------------------------------------------------------
//	Count Modula - Custom components
//----------------------------------------------------------------------------
#include "componentlibrary.hpp"
//#include "rack.hpp"

using namespace rack;

//-------------------------------------------------------------------
// screws
//-------------------------------------------------------------------
struct CountModulaScrew : SVGScrew {
	CountModulaScrew() {
		sw->setSVG(SVG::load(assetPlugin(plugin, "res/Components/ScrewHex.svg")));
		box.size = sw->box.size;
	}
};

//-------------------------------------------------------------------
// Ports
//-------------------------------------------------------------------
struct CountModulaJack : SVGPort {
	CountModulaJack() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/Jack.svg")));
	}
};

//-------------------------------------------------------------------
// Knobs
//-------------------------------------------------------------------
// TODO: parameterise the colour

// base knob
struct CountModulaKnob : SVGKnob {
	CountModulaKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};

// coloured knobs
struct CountModulaKnobRed : CountModulaKnob {
	CountModulaKnobRed() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobRed.svg")));
	}
};

struct CountModulaKnobOrange : CountModulaKnob {
	CountModulaKnobOrange() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobOrange.svg")));
	}
};

struct CountModulaKnobYellow : CountModulaKnob {
	CountModulaKnobYellow() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobYellow.svg")));
	}
};

struct CountModulaKnobGreen : CountModulaKnob {
	CountModulaKnobGreen() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobGreen.svg")));
	}
};

struct CountModulaKnobBlue : CountModulaKnob {
	CountModulaKnobBlue() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobBlue.svg")));
	}
};

struct CountModulaKnobGrey : CountModulaKnob {
	CountModulaKnobGrey() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobGrey.svg")));
	}
};

struct CountModulaKnobWhite : CountModulaKnob {
	CountModulaKnobWhite() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/KnobWhite.svg")));
	}
};

//-------------------------------------------------------------------
// rotary switches
//-------------------------------------------------------------------
// TODO: parameterise the colour
struct CountModulaRotarySwitchRed : CountModulaKnobRed {
	CountModulaRotarySwitchRed() {
		snap = true;
		smooth = false;
	}
};

struct CountModulaRotarySwitchOrange : CountModulaKnobOrange {
	CountModulaRotarySwitchOrange() {
		snap = true;
		smooth = false;
	}
};

struct CountModulaRotarySwitchYellow : CountModulaKnobYellow {
	CountModulaRotarySwitchYellow() {
		snap = true;
		smooth = false;
	}
};

struct CountModulaRotarySwitchGreen : CountModulaKnobGreen {
	CountModulaRotarySwitchGreen() {
		snap = true;
		smooth = false;
	}
};

struct CountModulaRotarySwitchBlue : CountModulaKnobBlue {
	CountModulaRotarySwitchBlue() {
		snap = true;
		smooth = false;
	}
};


struct CountModulaRotarySwitchGrey : CountModulaKnobGrey {
	CountModulaRotarySwitchGrey() {
		snap = true;
		smooth = false;
	}
};

struct CountModulaRotarySwitchWhite : CountModulaKnobWhite {
	CountModulaRotarySwitchWhite() {
		snap = true;
		smooth = false;
	}
};

//-------------------------------------------------------------------
// 5 pos rotary switches
//-------------------------------------------------------------------
// TODO: parameterise the number of positions and the colour

struct CountModulaRotarySwitch5PosRed : CountModulaRotarySwitchRed {
	CountModulaRotarySwitch5PosRed() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosOrange : CountModulaRotarySwitchOrange {
	CountModulaRotarySwitch5PosOrange() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosYellow : CountModulaRotarySwitchYellow {
	CountModulaRotarySwitch5PosYellow() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosGreen : CountModulaRotarySwitchGreen {
	CountModulaRotarySwitch5PosGreen() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosBlue : CountModulaRotarySwitchBlue {
	CountModulaRotarySwitch5PosBlue() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosGrey : CountModulaRotarySwitchGrey {
	CountModulaRotarySwitch5PosGrey() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

struct CountModulaRotarySwitch5PosWhite : CountModulaRotarySwitchWhite {
	CountModulaRotarySwitch5PosWhite() {
		minAngle = -0.4*M_PI;
		maxAngle = 0.4*M_PI;
	}
};

//-------------------------------------------------------------------
// on-off toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle2P : SVGSwitch, ToggleSwitch {
	CountModulaToggle2P() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/SW_Toggle_0.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/SW_Toggle_2.svg")));
	}
};

//-------------------------------------------------------------------
// on-off-on toggle switch
//-------------------------------------------------------------------
struct CountModulaToggle3P : SVGSwitch, ToggleSwitch {
	CountModulaToggle3P() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/SW_Toggle_0.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/SW_Toggle_1.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/SW_Toggle_2.svg")));
	}
};

//-------------------------------------------------------------------
// push button base
//-------------------------------------------------------------------
struct CountModulaPB :  SVGSwitch, ToggleSwitch {
	CountModulaPB() {
	}

	// override the base randomizer as it sets switches to invalid values.
	void randomize() override {
		SVGSwitch::randomize();
		if (value > 0.5f)
			setValue(1.0f);
		else
			setValue(0.0f);
	}
};

//-------------------------------------------------------------------
// push button
//-------------------------------------------------------------------
struct CountModulaPBSwitch : CountModulaPB {
    CountModulaPBSwitch() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButton_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButton_1.svg")));
    }
};

struct CountModulaPBSwitchMomentary : SVGSwitch, MomentarySwitch {
    CountModulaPBSwitchMomentary() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButton_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButton_1.svg")));
    }
};
 
//-------------------------------------------------------------------
// big square push button
//-------------------------------------------------------------------
struct CountModulaPBSwitchBig : CountModulaPB {
    CountModulaPBSwitchBig() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonBig_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonBig_1.svg")));
    }
};

struct CountModulaPBSwitchBigMomentary : SVGSwitch, MomentarySwitch {
    CountModulaPBSwitchBigMomentary() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonBig_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonBig_1.svg")));
    }
};
 
//-------------------------------------------------------------------
// really big square push button
//-------------------------------------------------------------------
struct CountModulaPBSwitchMega : CountModulaPB {
    CountModulaPBSwitchMega() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonMega_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonMega_1.svg")));
    }
};

struct CountModulaPBSwitchMegaMomentary : SVGSwitch, MomentarySwitch {
    CountModulaPBSwitchMegaMomentary() {
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonMega_0.svg")));
        addFrame(SVG::load(assetPlugin(plugin, "res/Components/PushButtonMega_1.svg")));
    }
};