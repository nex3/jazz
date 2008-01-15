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
    (1/(5 + -5) == Infinity) &&
    ("foo" + 1 == "foo1") &&
    ("" + undefined == "undefined") &&
    (true + "" == "true") &&
    ("" + false == "false") &&
    ("" + NaN == "NaN") &&
    ("" + 0 == "0") &&
    ("" + (-0) == "0") &&
    ("" + -15 == "-15") &&
    ("" + Infinity == "Infinity") &&
    ("" + 12 == "12") &&
    ("" + 123.456 == "123.456") &&
    ("" + 0.003 == "0.003") &&
    ("" + 1e27 == "1e+27") &&
    ("" + 1e-8 == "1e-8") &&
    (+("" + -1.5555555555555555e-100) == -1.5555555555555555e-100);