var a = {};
for (var i = 0; i < 100; i++) {
  a[i] = i;
}

var res = true;
for (i = 0; i < 100; i++) {
  res = res && (a[i + ""] == i);
}

var f = function() { return ({}); };
res = res && f() != f();

return res;
