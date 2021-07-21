//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - header.
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "rack.hpp"
using namespace rack;

// Forward-declare the Plugin
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
#include "DeclareModels.hpp"

// settings file 
json_t * readSettings();
void saveSettings(json_t *rootJ);

// settings functions
int readDefaultIntegerValue(std::string);
void saveDefaultIntegerValue(std::string setting, int value);

#include "components/CountModulaKnobs.hpp"
#include "components/CountModulaComponents.hpp"
#include "components/CountModulaPushButtons.hpp"
#include "components/StdComponentPositions.hpp"


