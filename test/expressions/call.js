blargle = isNaN;
a = {};
a[1] = blargle;
return isNaN(NaN) && !isNaN(1) &&
  blargle(NaN) && !blargle(2) &&
  a[1](NaN) && !a[1](3);
