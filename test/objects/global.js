a = 12;
this["b"] = 17;
this["c"] = 19;
c += 20;
return (this["a"] == 12) &&
  (b == 17) &&
  (c == 39);
