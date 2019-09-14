//----------------------------------------------------------------------------
//	/^M^\ Count Modula - theme selection dataFromJson
// common  functionality for handling loading of selected theme
//----------------------------------------------------------------------------

json_t* jsonTheme = json_object_get(root, "theme");

if (jsonTheme)
	currentTheme = json_integer_value(jsonTheme);
else
	currentTheme = 0;