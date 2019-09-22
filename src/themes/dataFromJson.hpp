//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme selection dataFromJson
// common  functionality for handling loading of selected theme
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

json_t* jsonTheme = json_object_get(root, "theme");

if (jsonTheme)
	currentTheme = json_integer_value(jsonTheme);
else
	currentTheme = 0;