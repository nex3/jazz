load("test/test.js");

test("Basic bitwise and should work.", function() {
  assertEqual(0, 1 & 2);
  assertEqual(3, 7 & 3);
});

test("A negative integer & a positive integer should be positive.", function() {
  assertEqual(3, -5 & 7);
});

test("A negative integer & a negative integer should be negative.", function() {
  assertEqual(-4, -3 & -2);
});

test("A 32-bit number & a 2-bit number should give a 2-bit number.", function() {
  assertEqual(2, 2147483650 & 2);
});

test("A 32-bit number & a negative 2-bit number should give a negative 32-bit number.", function() {
  assertEqual(-2147483646, 2147483650 & -2);
});

test("A 36-bit number & a 2-bit number should give a 2-bit number.", function() {
  assertEqual(0, 34359738385 & 2);
});

test("A 36-bit number & a negative 2-bit number should treat the negative as a higher-order bit.", function() {
  assertEqual(16, 34359738385 & -2);
});

test("A 36-bit number & another 36-bit number should truncate the two numbers before &-ing.", function() {
  assertEqual(17, 34359738385 & 34359738385);
});

test("& should have higher precedence than |.", function() {
  assertEqual((7 & 3) | (-5 & 12), 7 & 3 | -5 & 12);
});

test("& should have higher precedence than ^.", function() {
  assertEqual((7 & 3) ^ (-5 & 12), 7 & 3 ^ -5 & 12);
});

runTests();
