load("test/test.js");

test("A normal global function call should work", function() {
  assert(isNaN(NaN));
  assert(!isNaN(1));
});

test("A function should be able to be assigned to a variable", function() {
  var blargle = isNaN;
  assert(blargle(NaN));
  assert(!blargle(2));
});

test("A function should be able to be assigned to a property", function() {
  var a = {};
  a[1] = isNaN;
  assert(a[1](NaN));
  assert(!a[1](3));
});

runTests();
