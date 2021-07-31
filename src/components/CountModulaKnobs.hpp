//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Custom knobs
//  Copyright (C) 2021  Adam Verspaget 
//----------------------------------------------------------------------------
#include "componentlibrary.hpp"

using namespace rack;

//--------------------------------------------------------------------------------------------------
// knob definitions
//--------------------------------------------------------------------------------------------------
// base knob
struct CountModulaKnob : SvgKnob {
	std::string svgFile = "";
	float orientation = 0.0;
	
	CountModulaKnob() {
		svgFile = "";
		orientation = 0.0;
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};

// red knob
template <typename TBase = CountModulaKnob>
struct TRedKnob : TBase {
	TRedKnob() {
		this->svgFile = "Red.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TRedKnob<> RedKnob;

// orange knob
template <typename TBase = CountModulaKnob>
struct TOrangeKnob : TBase {
	TOrangeKnob() {
		this->svgFile = "Orange.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TOrangeKnob<> OrangeKnob;

// yellow knob
template <typename TBase = CountModulaKnob>
struct TYellowKnob : TBase {
	TYellowKnob() {
		this->svgFile = "Yellow.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TYellowKnob<> YellowKnob;

// green knob
template <typename TBase = CountModulaKnob>
struct TGreenKnob : TBase {
	TGreenKnob() {
		this->svgFile = "Green.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TGreenKnob<> GreenKnob;

// blue knob
template <typename TBase = CountModulaKnob>
struct TBlueKnob : TBase {
	TBlueKnob() {
		this->svgFile = "Blue.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TBlueKnob<> BlueKnob;

// violet knob
template <typename TBase = CountModulaKnob>
struct TVioletKnob : TBase {
	TVioletKnob() {
		this->svgFile = "Violet.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TVioletKnob<> VioletKnob;

// grey knob
template <typename TBase = CountModulaKnob>
struct TGreyKnob : TBase {
	TGreyKnob() {
		this->svgFile = "Grey.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TGreyKnob<> GreyKnob;

// white knob
template <typename TBase = CountModulaKnob>
struct TWhiteKnob : TBase {
	
	TWhiteKnob() {
		this->svgFile = "White.svg";
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/Knob" + this->svgFile)));
	}
};
typedef TWhiteKnob<> WhiteKnob;

//--------------------------------------------------------------------------------------------------
// Knob sizes
//--------------------------------------------------------------------------------------------------
// Small knob
template <typename TBase>
struct SmallKnob : TBase {
	SmallKnob() {
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobSmall" + this->svgFile)));
	}
};

// Mega knob
template <typename TBase>
struct MegaKnob : TBase {
	MegaKnob() {
		this->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Components/KnobMega" + this->svgFile)));
	}
};

// Knob Orientations
template <typename TBase>
struct Left90 : TBase {
	Left90() {
		this->orientation = - M_PI/2;
	}
};

template <typename TBase>
struct Right90 : TBase {
	Right90() {
		this->orientation = M_PI/2;
	}
};

//--------------------------------------------------------------------------------------------------
// Knob angles of operation
//--------------------------------------------------------------------------------------------------
// 120 degree angle of operation
template <typename TBase>
struct OperatingAngle120 : TBase {
	OperatingAngle120() {
		this->minAngle = -0.33*M_PI + this->orientation;
		this->maxAngle = 0.33*M_PI + this->orientation;
	}
};

// 145 degree angle of operation
template <typename TBase>
struct OperatingAngle145 : TBase {
	OperatingAngle145() {
		this->minAngle = -0.4*M_PI + this->orientation;
		this->maxAngle = 0.4*M_PI + this->orientation;
	}
};

// 210 degree angle of operation
template <typename TBase>
struct OperatingAngle180 : TBase {
	OperatingAngle180() {
		this->minAngle = -0.5*M_PI + this->orientation;
		this->maxAngle = 0.5*M_PI + this->orientation;
	}
};

// 210 degree angle of operation
template <typename TBase>
struct OperatingAngle210 : TBase {
	OperatingAngle210() {
		this->minAngle = -0.58*M_PI + this->orientation;
		this->maxAngle = 0.58*M_PI + this->orientation;
	}
};

// 240 degree angle of operation
template <typename TBase>
struct OperatingAngle240 : TBase {
	OperatingAngle240() {
		this->minAngle = -0.68*M_PI + this->orientation;
		this->maxAngle = 0.68*M_PI + this->orientation;
	}
};

// 270 degree angle of operation
template <typename TBase>
struct OperatingAngle270 : TBase {
	OperatingAngle270() {
		this->minAngle = -0.75*M_PI + this->orientation;
		this->maxAngle = 0.75*M_PI + this->orientation;
	}
};

// 300 degree angle of operation
template <typename TBase>
struct OperatingAngle300 : TBase {
	OperatingAngle300() {
		this->minAngle = -0.83*M_PI + this->orientation;
		this->maxAngle = 0.83*M_PI  + this->orientation;
	}
};

