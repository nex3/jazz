var a = {};
var b = {};
var c = {};
a["bar"] = 12;
b["a"] = a;
c.i = 1;
c["i"] *= 5;
return (a["foo"] == undefined) &&
  (a.foo == undefined) &&
  (a.bar == 12) &&
  (a["bar"] == 12) &&
  (a["ba" + "r"] == 12) &&
  (b["a"] == a) &&
  (b["a"]["bar"] == 12) &&
  ((a["baz"] = 12) == 12) &&
  (a["baz"] == 12) &&
  (c["i"] == 5) &&
  ((c["i"] += 2) == 7);
