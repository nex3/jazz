load("test/test.js");

test("Basic number parsing should work", function() {
  assertEqual(2, 1 + 1);
});

test("Trailing decimal points should be ignored", function() {
  assertEqual(12, 12.);
});

test("An exponent of 0 should be ignored", function() {
  assertEqual(12, 12e0);
});

test("An exponent of 1 should multiply a number by 10", function() {
  assertEqual(120, 12e1);
});

test("Exponents should work with decimal numbers", function() {
  assertEqual(1212, 12.12e2);
});

test("Decimal numbers with a leading decimal should work", function() {
  assertEqual(0.1, .1);
});

test("Decimal numbers with a leading decimal should work with exponents", function() {
  assertEqual(200, .2e3);
});

test("Negative exponents should appropriately divide the number", function() {
  assertEqual(1, 100e-2);
});

test("Hexadecimal numbers beginning with 0x should be parsed correctly", function() {
  assertEqual(11256099, 0xabc123);
});

test("Hexadecimal numbers should be able to mix upper and lower case", function() {
  assertEqual(4550127, 0X456DEF);
});

runTests();
