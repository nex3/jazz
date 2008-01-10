var a = NaN + 1;
var b = 1 + NaN;
var c = NaN + NaN;
var d = Infinity + -Infinity;
return ((1 + 1) == 2) &&
    ((1.6 + 5) == 6.6) &&
    ((5 + -2) == 3) &&
    ((true + 3) == 4) &&
    ((3 + true) == 4) &&
    ((false + 3) == 3) &&
    (a != a) && (b != b) && (c != c) && (d != d) &&
    ((Infinity + Infinity) == Infinity) &&
    ((-Infinity + -Infinity) == -Infinity) &&
    ((Infinity + 12) == Infinity) &&
    ((-Infinity + 999) == -Infinity) &&
    (1/(-0 + -0) == -Infinity) &&
    (1/(0 + 0) == Infinity) &&
    (1/(0 + -0) == Infinity) &&
    (1/(-0 + 0) == Infinity) &&
    ((0 + 15) == 15) &&
    ((-197 + -0) == -197) &&
    ((5 + -5) == 0) &&
    (1/(5 + -5) == Infinity);
    
