var a = NaN * 1;
var b = 1 * NaN;
var c = NaN * NaN;
var d = Infinity * -0;
var e = -Infinity * 0;
return ((3 * 4) == 12) &&
  ((1.3 * -3.7) == -4.81) &&
  (a != a) && (b != b) && (c != c) && (d != d) && (e != e) &&
  ((Infinity * Infinity) == Infinity) &&
  ((-Infinity * -Infinity) == Infinity) &&
  ((-Infinity * Infinity) == -Infinity) &&
  ((12 * Infinity) == Infinity) &&
  ((12 * -Infinity) == -Infinity) &&
  ((-Infinity * -0.001) == Infinity);
