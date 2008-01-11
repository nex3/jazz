var a = NaN % NaN;
var b = NaN % 10;
var c = -12 % NaN;
var d = NaN % Infinity;
var e = Infinity % 12;
var f = -13 % 0;
var g = Infinity % Infinity;
var h = 0 % 0;

var float_error = 1e-15;
var i = 1 % .2;
var j = (1 % .3) - .1;
var k = (1 % -.3) - .1;
var l = (-1 % -.3) + .1;
var m = (-1 % .3) + .1;
return (a != a) && (b != b) && (c != c) && (d != d) &&
    (e != e) && (f != f) && (g != g) && (h != h) &&
    (12 % Infinity == 12) &&
    (0 % Infinity == 0) &&
    (0 % 178 == 0) &&
    (8 % 3 == 2) &&
    (12 % 186 == 12) &&
    (-8 % -3 == -2) &&
    (i < float_error && i > -float_error) &&
    (j < float_error && j > -float_error) &&
    (k < float_error && k > -float_error) &&
    (l < float_error && l > -float_error) &&
    (m < float_error && m > -float_error);
