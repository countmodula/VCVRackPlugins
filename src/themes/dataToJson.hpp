//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme selection dataToJson
// common  functionality for handling saving of selected theme
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
json_object_set_new(root, "theme", json_integer(currentTheme));
