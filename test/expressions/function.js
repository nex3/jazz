load("test/test.js");

test("A simple anonymous function should work", function() {
  assertEqual(12, (function() { return 12; })());
});

test("An anonymous function should be able to shadow outside variables", function() {
  var a = "hello";
  (function() { var a = 17; return a + 1; })();
  assertEqual("hello", a);
});

test("An anonymous function should be able to modify outside variables", function() {
  var a = "hello";
  (function() { a = 17; return a + 1; })();
  assertEqual(17, a);
});

// Should this be in objects/function.js?
test("A function object should equal itself", function() {
  assertEqual(isNaN, isNaN);
  var c = function() { return 12; };
  assertEqual(c, c);
});

test("A function object shouldn't be equal to another with the same body", function() {
  assertNotEqual(function() {}, function() {});
});

test("A function without an explicit return should return `undefined'", function() {
  assertEqual(undefined, (function() { 15; })());
});

test("Closures should be able to get and set out-of-scope variables", function() {
  var state = (function() {
    var b;
    return function(a) {
      return a ? function() { return b; } : function (a) { b = a; };
    };
  })();
  var get = state(true);
  var set = state(false);

  assertEqual(undefined, get());
  set(12);
  assertEqual(12, get());
});

test("Function parameters that aren't given should be undefined", function() {
  assertEqual(undefined, (function(a) { return a; })());
  assertEqual(undefined, (function(a, b, c) { return b; })(1));
});

test("Functions should ignore extra parameters", function() {
  assertEqual(1, (function(a) { return a; })(1, 2, 3, 4, 5));
});

test("Closures should be able to get and set properties without conflicts", function() {
  var Obj = function() {
    var obj = {};
    obj.set = function(name, val) { obj[name] = val; };
    obj.get = function(name) { return obj[name]; };
    return obj;
  };
  var o1 = Obj();
  var o2 = Obj();

  o1.set("foo", "bar");
  assertEqual("bar", o1.get("foo"));
  assertEqual(undefined, o2.get("foo"));
  o2.set("foo", 15);
  assertEqual(15, o2.get("foo"));
  assertEqual("bar", o1.get("foo"));
});

runTests();
