load("test/test.js");

test("Basic integer multiplication should work.", function() {
  assertEqual(12, 3 * 4);
});

test("Decimal multiplication should work.", function() {
  assertEqual(5.2, 1.3 * 4);
});

test("Multiplication involving negative numbers should work.", function() {
  assertEqual(-10, 5 * -2);
});

test("NaN times anything should be NaN.", function() {
  assertNaN(NaN * 1);
  assertNaN(1 * NaN);
  assertNaN(NaN * NaN);
});

test("Infinity times 0 should be NaN.", function() {
  assertNaN(Infinity * 0);
  assertNaN(-0 * Infinity);
  assertNaN(-Infinity * 0);
});

test("Infinity times infinity should be infinity.", function() {
  assertEqual(Infinity, Infinity * Infinity);
});

test("Negative infinity times negative infinity should be infinity.", function() {
  assertEqual(Infinity, -Infinity * -Infinity);
});

test("Infinity times negative infinity should be negative infinity.", function() {
  assertEqual(-Infinity, -Infinity * Infinity);
});

test("Infinity times a normal number should be infinity.", function() {
  assertEqual(Infinity, 12 * Infinity);
});

test("Negative infinity times a normal number should be negative infinity.", function() {
  assertEqual(-Infinity, 0.01 * -Infinity);
});

test("Negative infinity times a negative number should be infinity.", function() {
  assertEqual(Infinity, -2 * -Infinity);
});

test("Infinity times a negative number should be negative infinity.", function() {
  assertEqual(-Infinity, -5 * Infinity);
});

runTests();
