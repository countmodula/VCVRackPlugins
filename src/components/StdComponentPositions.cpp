//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard column and row functions
//  Copyright (C) 2019  Adam Verspaget
//------------------------------------------------------------------------
#include "StdComponentPositions.hpp"

//------------------------------------------------------------------------
// Row position functions
//------------------------------------------------------------------------

int STD_HALF_ROWS5(int row) {
	if (row >= STD_ROW5)
		return STD_ROWS5[STD_ROW5];
	else
		return (STD_ROWS5[row]  + STD_ROWS5[row + 1]) / 2;
}


int STD_HALF_ROWS6(int row) {
	if (row >= STD_ROW6)
		return STD_ROWS6[STD_ROW6];
	else
		return (STD_ROWS6[row]  + STD_ROWS6[row + 1]) / 2;
}

int STD_HALF_ROWS7(int row) {
	if (row >= STD_ROW7)
		return STD_ROWS7[STD_ROW7];
	else
		return (STD_ROWS7[row] + STD_ROWS7[row + 1]) / 2;
}

int STD_HALF_ROWS8(int row) {
	if (row >= STD_ROW8)
		return STD_ROWS8[STD_ROW8];
	else
		return (STD_ROWS8[row] + STD_ROWS8[row + 1]) / 2;
}
