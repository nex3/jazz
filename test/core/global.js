load("test/test.js");

test("A loaded file should be able to write to the global state.", function() {
  load("test/core/_to_load.js");
  assertEqual("bar", foo);
});

test("isNaN should return true for NaN", function() {
  assert(isNaN("foobar"));
});

test("isNaN should return true for values that are coerced to NaN", function() {
  assert(isNaN("foobar"));
});

test("isNaN should return false for values that aren't coerced to NaN", function() {
  assert(!isNaN("12"));
  assert(!isNaN(false));
});

test("isNaN should return true for no arguments", function() {
  assert(isNaN());
});

runTests();
