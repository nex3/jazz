var a = NaN / 1;
var b = 1 / NaN;
var c = NaN / NaN;
var d = Infinity / Infinity;
var e = Infinity / -Infinity;
var f = -Infinity / Infinity;
var g = -Infinity / -Infinity;
var h = 0/0;
return ((4 / 2) == 2) &&
    (3 / 4 == 0.75) &&
    (-18 / 9 == -2) &&
    (a != a) && (b != b) && (c != c) && (e != e) && (f != f) && (g != g) &&
    (Infinity / 0 == Infinity) &&
    (-Infinity / 0 == -Infinity) &&
    (-Infinity / -0 == Infinity) &&
    (Infinity / -0 == -Infinity) &&
    (Infinity / 12 == Infinity) &&
    (-Infinity / -12 == Infinity) &&
    (5 / Infinity == 0) &&
    (1/(-5 / Infinity) == -Infinity) &&
    (h != h) &&
    (13/0 == Infinity) &&
    (-1/0 == -Infinity) &&
    (1/-0 == -Infinity);
