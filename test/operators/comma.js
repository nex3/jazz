var a = 1;
var b = -1;
return ((a = 2, a) == 2) &&
    ((a = 3, a = 4, a) == 4) &&
    ((a = 5, b = 6, a + b) == 11);
