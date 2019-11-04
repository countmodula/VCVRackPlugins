//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Fade expander message
//	For passing fade details to and from the fade and fade expander modules
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

struct FadeExpanderMessage {
	float envelope = 0.0f;
	bool run = false;
	bool fadeIn = false;
	bool fadeOut = false;
};

