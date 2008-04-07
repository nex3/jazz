load("test/test.js");

test("An undefined property should return `undefined'", function() {
  assertEqual(undefined, {}.foo);
});

test("A defined property should return its value", function() {
  var obj = {};

  obj.foo = 12;
  assertEqual(12, obj.foo);
});

test("Bracket-notation and dot-notation property access should both work", function() {
  var obj = {};

  assertEqual(undefined, obj.foo);
  assertEqual(undefined, obj["foo"]);

  obj.foo = "bar";
  assertEqual("bar", obj.foo);
  assertEqual("bar", obj["foo"]);

  obj["bar"] = "foo";
  assertEqual("foo", obj.bar);
  assertEqual("foo", obj["bar"]);
});

test("Expressions should be nestable within member-expression brackets", function() {
  var obj = {};

  obj["ba" + "r"] = 12;
  assertEqual(12, obj.bar);
});

test("Member expressions should be chainable", function() {
  var obj = {};
  obj.foo = {};
  obj.foo.bar = {};
  obj.foo.bar.baz = 12;

  assertEqual(12, obj.foo["bar"].baz);
});

test("Member expressions should work with operator-equals", function() {
  var obj = {};
  obj.foo = 1;
  obj.foo += 2;
  assertEqual(3, obj.foo);
});

test("Member assignment expressions should return their values", function() {
  var obj = {};

  assertEqual(12, obj.foo = 12);
  assertEqual(13, obj["bar"] = 13);
});

runTests();
