load("test/test.js");

test("Assigning to a variable should set that variable's value.", function() {
  var a = 1;
  assertEqual(1, a);
});

test("Assigning to an already-assigned variable should reset that variable's value.", function() {
  var a = 1;
  a = 2;
  assertEqual(2, a);
});

test("The assignment operator should return the result of the assignment.", function() {
  var a;
  assertEqual(1, a = 1);
});

test("Operator-assignment should return the result of the assignment.", function() {
  var a = 2;
  assertEqual(6, a *= 3);
});

test("Times-equals should multiply and assign.", function() {
  var a = 2;
  a *= 3;
  assertEqual(6, a);
});

test("Div-equals should divide and assign.", function() {
  var a = 6;
  a /= 2;
  assertEqual(3, a);
});

test("Mod-equals should modulo and assign.", function() {
  var a = 5;
  a %= 3;
  assertEqual(2, a);
});

test("Plus-equals should add and assign.", function() {
  var a = 1;
  a += 7;
  assertEqual(8, a);
});

test("Minus-equals should subtract and assign.", function() {
  var a = 8;
  a -= 12;
  assertEqual(-4, a);
});

test("Lshift-equals should shift left and assign.", function() {
  var a = -4;
  a <<= 2;
  assertEqual(-16, a);
});

test("Rshift-equals should shift right and assign.", function() {
  var a = -16;
  a >>= 1;
  assertEqual(-8, a);
});

test("Unisgned rhift-equals should shift right unsigned and assign.", function() {
  var a = -8;
  a >>>= 2;
  assertEqual(1073741822, a);
});

test("And-equals should bitwise and and assign.", function() {
  var a = 1073741822;
  a &= 15;
  assertEqual(14, a);
});

test("Xor-equals should bitwise xor and assign.", function() {
  var a = 14;
  a ^= 7;
  assertEqual(9, a);
});

test("Or-equals should bitwise or and assign.", function() {
  var a = 9;
  a |= 6;
  assertEqual(15, a);
});

runTests();
