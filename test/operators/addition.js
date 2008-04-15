load("test/test.js");

test("Basic integer addition should work.", function() {
  assertEqual(2, 1 + 1);
});

test("Decimal addition should work.", function() {
  assertEqual(6.6, 1.6 + 5);
});

test("Addition involving negative numbers should work.", function() {
  assertEqual(3, 5 + -2);
});

test("Addition should coerce its first argument to a number if the second is a number.", function() {
  assertEqual(4, true + 3);
});

test("Addition should coerce its second argument to a number if the first is a number.", function() {
  assertEqual(4, 3 + true);
});

test("Addition should coerce both arguments to numbers if neither is a string.", function() {
  assertEqual(1, true + false);
  assertNaN(undefined + null);
});

test("NaN plus anything should be NaN.", function() {
  assertNaN(NaN + 1);
  assertNaN(1 + NaN);
  assertNaN(NaN + NaN);
});

test("Infinity plus negative infinity should be NaN.", function() {
  assertNaN(Infinity + -Infinity);
});

test("Infinity plus infinity should be infinity.", function() {
  assertEqual(Infinity, Infinity + Infinity);
});

test("Negative infinity plus negative infinity should be negative infinity.", function() {
  assertEqual(-Infinity, -Infinity + -Infinity);
});

test("Infinity plus a normal number should be infinity.", function() {
  assertEqual(Infinity, Infinity + 12);
});

test("Negative infinity plus a normal number should be negative infinity.", function() {
  assertEqual(-Infinity, -Infinity + 999);
});

test("Negative zero plus negative zero should be negative zero.", function() {
  assertNegative0(-0 + -0);
});

test("Positive zero plus positive zero should be positive zero.", function() {
  assertPositive0(0 + 0);
});

test("Positive zero plus negative zero should be positive zero.", function() {
  assertPositive0(0 + -0);
});

test("Zero plus anything should equal the other number.", function() {
  assertEqual(15, 0 + 15);
});

test("Negative zero plus anything should equal the other number.", function() {
  assertEqual(-197, -197 + -0);
});

test("A number plus its inverse should equal positive zero.", function() {
  assertPositive0(5 + -5);
});

test("A string plus a number should return the string concatenation of the two.", function() {
  assertEqual("foo1", "foo" + 1);
});

test("An empty string plus a string should return the other string.", function() {
  assertEqual("bar", "" + "bar");
});

test('The string conversion of undefined should be "undefined".', function() {
  assertEqual("undefined", "" + undefined);
});

test('The string conversion of true should be "true".', function() {
  assertEqual("true", "" + true);
});

test('The string conversion of 0 should be "0".', function() {
  assertEqual("0", "" + 0);
});

test('The string conversion of (-0) should be "0".', function() {
  assertEqual("0", "" + (-0));
});

test('The string conversion of -15 should be "-15".', function() {
  assertEqual("-15", "" + -15);
});

test('The string conversion of Infinity should be "Infinity".', function() {
  assertEqual("Infinity", "" + Infinity);
});

test('The string conversion of 12 should be "12".', function() {
  assertEqual("12", "" + 12);
});

test('The string conversion of 123.456 should be "123.456".', function() {
  assertEqual("123.456", "" + 123.456);
});

test('The string conversion of 0.003 should be "0.003".', function() {
  assertEqual("0.003", "" + 0.003);
});

test('The string conversion of 1e27 should be "1e+27".', function() {
  assertEqual("1e+27", "" + 1e27);
});

test('The string conversion of 1e-8 should be "1e-8".', function() {
  assertEqual("1e-8", "" + 1e-8);
});

test('The string conversion of null should be "null".', function() {
  assertEqual("null", "" + null);
});

test('The string conversion of {} should be "[object Object]".', function() {
  assertEqual("[object Object]", "" + {});
});

runTests();
