load("test/test.js");

test("Decrementing a variable should decrease its value by 1.", function() {
  var a = 1;
  a--;
  assertEqual(0, a);

  --a;
  assertEqual(-1, a);
});

test("Post-decrementing should return the original value.", function() {
  var a = 1;
  assertEqual(1, a--);
});

test("Pre-decrementing should return the new value.", function() {
  var a = 1;
  assertEqual(0, --a);
});

test("Decrementing should perform type conversion.", function() {
  var a = true;
  assertEqual(1, a--);
  assertEqual(0, a);
});

/*test("Decrementing should work on properties.", function() {
  var a = {};
  a.b = 1;
  a.b--;
  assertEqual(0, a.b);
});*/

runTests();
