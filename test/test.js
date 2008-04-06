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
  fn.desc = desc;
  push(tests, fn);
};

print = function(str) {
  if (str)
    write(str + "\n");
  else
    write("\n");
};

printFailed = function() {
  for (var i = 0; i < tests.failed.length; i++) {
    var f = tests.failed[i];
    print("Failed: " + f.desc);
    if (f.assertMessage !== true)
      print("  (" + f.assertMessage + ")");
  }
};

runTests = function() {
  for (var i = 0; i < tests.length; i++) {
    tests.assertFailed = false;
    var f = tests[i];
    f();
    if (tests.assertFailed) {
      write("F");
      f.assertMessage = tests.assertFailed;
      push(tests.failed, f);
    } else write(".");
  }
  print();
  printFailed();
};

assert = function(result, msg) {
  tests.assertFailed = tests.assertFailed || (!result && (msg || true));
};

assertEqual = function(expected, actual) {
  assert(expected === actual, "Expected " + expected + ", was " + actual);
};
