//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Utilities
//	Handy little bits and bobs to make life easier
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#pragma once
// handy macros to convert a bool to an appropriate value for output and display
#define boolToGate(x) x ? 10.0f : 0.0f 
#define boolToLight(x) x ? 10.0f : 0.0f 
#define boolToAudio(x) x ? 5.0f : -5.0f
