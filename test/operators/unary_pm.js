load("test/test.js");

test("A unary plus should convert 15 to 15.", function() {
  assertEqual(15, +15);
});

test("A unary plus should convery -12 to -12.", function() {
  assertEqual(-12, +(-12));
});

test("A unary plus should convert true to 1.", function() {
  assertEqual(1, +true);
});

test("A unary plus should convert false to 0.", function() {
  assertEqual(0, +false);
});

test("A unary plus should convert undefined to NaN.", function() {
  assertNaN(+undefined);
});

test("A unary plus should convert an object to NaN.", function() {
  assertNaN(+{});
});

test("A unary plus should convert a non-number string to NaN.", function() {
  assertNaN(+"foo");
});

test("A unary plus should convert a number string to that number.", function() {
  assertEqual(12, +"12");
});

test("A unary plus should strip whitespace from strings before converting them.", function() {
  assertEqual(12, +" \n \u205F 12 \n ");
});

test("A unary plus should understand decimals and exponents in strings.", function() {
  assertEqual(1.3e6, +"1.3e6");
});

test("A unary plus should understand an initial minus sign.", function() {
  assertEqual(-12, +"-12");
});

test("A unary plus should ignore an initial plus sign.", function() {
  assertEqual(12, +"12");
});

test("A unary plus should convert the empty string to 0.", function() {
  assertEqual(0, +"");
});

test("A unary plus should convert an all-whitespace string to 0.", function() {
  assertEqual(0, +"  \n ");
});

test("A unary plus should convert null to 0.", function() {
  assertEqual(0, +null);
});

test("A unary minus should convert positive numbers to negative.", function() {
  assertEqual(-12, -(12));
});

test("A unary minus should convert negative numbers to positive.", function() {
  assertEqual(180, -(-180));
});

test("A unary minus should perform type conversion.", function() {
  assertEqual(-1, -true);
});

runTests();
