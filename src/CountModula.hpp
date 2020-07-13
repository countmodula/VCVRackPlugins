//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - header.
//  Copyright (C) 2019  Adam Verspaget
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

// theme functions
int readDefaultTheme();
void saveDefaultTheme(int theme);

#include "components/CountModulaComponents.hpp"
#include "components/CountModulaPushButtons.hpp"
#include "components/StdComponentPositions.hpp"


