load("test/test.js");

test("Incrementing a variable should increase its value by 1.", function() {
  var a = 1;
  a++;
  assertEqual(2, a);

  ++a;
  assertEqual(3, a);
});

test("Post-incrementing should return the original value.", function() {
  var a = 1;
  assertEqual(1, a++);
});

test("Pre-incrementing should return the new value.", function() {
  var a = 1;
  assertEqual(2, ++a);
});

test("Incrementing should perform type conversion.", function() {
  var a = true;
  assertEqual(1, a++);
  assertEqual(2, a);
});

/*test("Incrementing should work on properties.", function() {
  var a = {};
  a.b = 1;
  a.b++;
  assertEqual(2, a.b);
});*/

runTests();
