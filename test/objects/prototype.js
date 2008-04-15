load("test/test.js");

// TODO: Change non-standard ({}).prototype tests to Object.prototype.

test("Setting a property on Object.prototype should make that property available to all objects.", function() {
  ({}).prototype.foo = 12;
  assertEqual(12, ({}).foo);
  assertEqual(12, isNaN.foo);
});

test("A prototype property should be overridden by local properties.", function() {
  ({}).prototype.bar = 12;
  var a = {};
  a.bar = 15;
  assertEqual(15, a.bar);
});

test("Two separate plain objects should have the same prototype.", function() {
  assertEqual(({}).prototype, ({}).prototype);
});

runTests();
