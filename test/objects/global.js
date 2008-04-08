load("test/test.js");

test("Variable declarations without `var' should work", function() {
  a = 12;
  assertEqual(12, a);
});

test("Variable declarations without `var' should declare global variables", function() {
  (function() { b = "foo"; })();
  assertEqual("foo", b);
});

test("The `this' object should provide access to global properties", function() {
  c = 12;
  assertEqual(12, this.c);
  assertEqual(12, this["c"]);

  this.d = "foo";
  assertEqual("foo", d);
});

test("The global object should equal itself", function() {
  assertEqual(this, this);
});

runTests();
