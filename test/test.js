Array = function() {
  var a = {};
  a.length = 0;
  return a;
};

push = function(arr, v) {
  arr[arr.length] = v;
  arr.length = arr.length + 1;
  return v;
};

tests = Array();
tests.failed = Array();
test = function(desc, fn) {
  f.desc = desc;
  push(tests, fn);
};

printFailed = function() {
  for (var i = 0; i < tests.failed.length; i++) {
    print("Failed: " + tests.failed[i].desc + "\n");
    if (f.assertMessage !== true)
      print("  (" + f.assertMessage + ")\n");
  }
};

runTests = function() {
  for (var i = 0; i < tests.length; i++) {
    tests.assertFailed = false;
    tests[i]();
    if (tests.assertFailed) {
      print("F");
      f.assertMessage = test.assertFailed;
      push(tests.failed, f);
    } else print(".");
  }
};

assert = function(result, msg) {
  tests.assertFailed = tests.assertFailed || (!result && (msg || true));
};

assertEqual = function(expected, actual) {
  assert(expected === actual, "Expected " + expected + ", was " + actual);
};
