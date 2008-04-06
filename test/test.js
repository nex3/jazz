Array = function() {
  var a = {};
  a.length = 0;
  a.push = function(v) {
    a[a.length] = v;
    a.length = a.length + 1;
    return v;
  };
  return a;
};

tests = Array();
tests.failed = Array();
test = function(desc, fn) {
  fn.desc = desc;
  tests.push(fn);
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
      tests.failed.push(f);
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
