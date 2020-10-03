load("test/test.js");

test("Object properties should be persistent", function() {
  var obj = {};

  obj.foo = 12;
  assertEqual(12, obj.foo);
});

test("Property keys should be converted to strings", function() {
  var a = {};

  a[1] = 1;
  assertEqual(1, a["1"]);
});

test("An object should be able to have many properties", function() {
  var a = {};
  var n = 1000;

  for (var i = 0; i < n; i++) {
    a[i] = i;
  }

  for (i = 0; i < n; i++) {
    assertEqual(i, a[i]);
  }
});

test("Object literals should be re-created whenever a function is run", function() {
  var f = function() { return ({}); };
  assertNotEqual(f(), f());
});

runTests();
