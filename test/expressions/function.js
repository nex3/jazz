var a = "hello";
this.b = "hello";
var c = function() {};
var state = (function() { var b; return function(a) {
  return a == 0 ? function() { return b; } : function (a) { b = a; };
};})();
var get = state(0);
var set = state(1);

var Obj = function() {
  var obj = {};
  obj.set = function(name, val) { obj[name] = val; };
  obj.get = function(name) { return obj[name]; };
  return obj;
};
var o1 = Obj();
var o2 = Obj();

return (function() { return 12; })() == 12 &&
  (function() { var a = 17; return a + 1; })() == 18 &&
  a == "hello" &&
  (function() { b = "goodbye"; })() == undefined &&
  b == "goodbye" &&
  c == c &&
  c != function() {1;} &&
  c() == undefined &&
  function() { b = "Woop!"; } &&
  b == "goodbye" &&
  ((function() { a = "blather"; })(), true) &&
  a == "blather" &&
  get() == undefined &&
  (set(12), get()) == 12 &&
  get() == 12 &&
  (set("Hello"), get()) == "Hello" &&
  (function(a) { return a; })() == undefined &&
  (function(a, b, c) { return b; })(1) == undefined &&
  (function(a) { return a; })(1, 2, 3, 4, 5) == 1 &&
  (o1.set("foo", "bar"), o1.get("foo")) == "bar" &&
  o2.get("foo") == undefined &&
  (o2.set("foo", 15), o2.get("foo")) == 15 &&
  o1.get("foo") == "bar";
