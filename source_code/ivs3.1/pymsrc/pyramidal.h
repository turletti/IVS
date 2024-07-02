#define MODE_1 0
#define MODE_2 1 
#define MODE_3 2 
#define MODE_4 3

#define same_value(val1, val2, delta) \
  (abs((int)(val1 - val2)) < (delta) ? 1 : 0)

#define put_mode(octet, MODE, i, j) \
  (MODE < MODE_3 ? (put_bit(octet, 0, i, j), \
		    (MODE == MODE_1 ? put_bit(octet, 0, i, j) : \
		    put_bit(octet, 1, i, j))) \
                 : (put_bit(octet, 1, i, j), \
		    (MODE == MODE_3 ? put_bit(octet, 0, i, j) : \
		    put_bit(octet, 1, i, j))))


static int tab_offset8[] = {
           0,    0,    0,    0,    2,    0,    0,    0, 
          16,    0,    0,    0,   18,    0,    0,    0, 
           4,    0,    0,    0,    6,    0,    0,    0, 
          20,    0,    0,    0,   22,    0,    0,    0, 
          32,    0,    0,    0,   34,    0,    0,    0, 
          48,    0,    0,    0,   50,    0,    0,    0, 
          36,    0,    0,    0,   38,    0,    0,    0, 
          52,    0,    0,    0,   54,    0,    0,    0 
};


static int tab_offset176[] = {
           0,    0,    0,    0,    2,    0,    0,    0, 
         352,    0,    0,    0,  354,    0,    0,    0, 
           4,    0,    0,    0,    6,    0,    0,    0, 
         356,    0,    0,    0,  358,    0,    0,    0, 
         704,    0,    0,    0,  706,    0,    0,    0, 
        1056,    0,    0,    0, 1058,    0,    0,    0, 
         708,    0,    0,    0,  710,    0,    0,    0, 
        1060,    0,    0,    0, 1062,    0,    0,    0 
};



static int tab_offset352[] = {
           0,    0,    0,    0,    2,    0,    0,    0, 
         704,    0,    0,    0,  706,    0,    0,    0, 
           4,    0,    0,    0,    6,    0,    0,    0, 
         708,    0,    0,    0,  710,    0,    0,    0, 
        1408,    0,    0,    0, 1410,    0,    0,    0, 
        2112,    0,    0,    0, 2114,    0,    0,    0, 
        1412,    0,    0,    0, 1414,    0,    0,    0, 
        2116,    0,    0,    0, 2118,    0,    0,    0 
};


static int hoffset352[] = {0, 4, 1408, 1412};

static int hoffset176[] = {0, 4, 704, 708};

static int hoffset8[] = {0, 4, 32, 36};

static int loffset352[] = {704, 708, 2112, 2116};

static int loffset176[] = {352, 356, 1056, 1060};

static int loffset8[] = {16, 20, 48, 52};

/*
  hoffset[0] = 0;
  hoffset[1] = 4;
  hoffset[2] = offset << 2;
  hoffset[3] = (offset << 2) + 4;
  loffset[0] = offset << 1;
  loffset[1] = (offset << 1) + 4;
  loffset[2] = 6*offset;
  loffset[3] = 6*offset + 4;
*/
