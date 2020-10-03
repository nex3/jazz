load("test/test.js");

test("True and true should be true.", function() {
  assertEqual(true, true && true);
});

test("True and false should be false.", function() {
  assertEqual(false, true && false);
  assertEqual(false, false && true);
});

test("False and false should be false.", function() {
  assertEqual(false, false && false);
});

test("And should return the second argument if the first is true.", function() {
  assertEqual(12, true && 12);
});

runTests();
