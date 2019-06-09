

//------------------------------------------------------------------------
// Standard column definitions
//------------------------------------------------------------------------
// column positions
const int STD_COLUMN_POSITIONS[32] = {
	30,
	60,
	90, // 8 HP
	120,
	150,
	180,
	210,
	240,
	270, // 16 HP
	300, 
	330,
	360,
	390,
	420,
	450,
	480,
	510,
	540,
	570,
	600,
	630,
	660,
	690,
	720,
	750,
	780,
	810,
	840,
	870,
	900,
	930,
	960
};

// column indices
const int STD_COL1 = 0;
const int STD_COL2 = 1;
const int STD_COL3 = 2;
const int STD_COL4 = 3;
const int STD_COL5 = 4;
const int STD_COL6 = 5;
const int STD_COL7 = 6;
const int STD_COL8 = 7;
const int STD_COL9 = 8;
const int STD_COL10 = 9;
const int STD_COL11 = 10;
const int STD_COL12 = 11;
const int STD_COL13 = 12;
const int STD_COL14 = 13;
const int STD_COL15 = 14;
const int STD_COL16 = 15;
const int STD_COL17 = 16;
const int STD_COL18 = 17;
const int STD_COL19 = 18;
const int STD_COL20 = 19;
const int STD_COL21 = 20;
const int STD_COL22 = 21;
const int STD_COL23 = 22;
const int STD_COL24 = 23;
const int STD_COL25 = 24;
const int STD_COL26 = 25;
const int STD_COL27 = 26;
const int STD_COL28 = 27;
const int STD_COL29 = 28;
const int STD_COL30 = 29;
const int STD_COL31 = 30;
const int STD_COL32 = 31;


//------------------------------------------------------------------------
// standard row definitions
//------------------------------------------------------------------------

// 6 row panel positions
const int STD_ROWS6[6] = {
	53,
	108,
	163,
	218,
	273,
	328
};

// 7 row panel positions
const int STD_ROWS7[7] = {
	50,
	96,
	142,
	188,
	234,
	280,
	326
};

// 8 row panel positions
const int STD_ROWS8[8] = {
	43,
	85,
	127,
	169,
	211,
	253,
	295,
	337
};	
	
// row indices
const int STD_ROW1 = 0;
const int STD_ROW2 = 1;
const int STD_ROW3 = 2;
const int STD_ROW4 = 3;
const int STD_ROW5 = 4;
const int STD_ROW6 = 5;
const int STD_ROW7 = 6;
const int STD_ROW8 = 7;


// half row calculations
int STD_HALF_ROWS6(int row);
int STD_HALF_ROWS7(int row);
int STD_HALF_ROWS8(int row);
