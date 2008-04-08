load("test/test.js");

test("Double quotes should be identical to single quotes", function() {
  assertEqual("foo", 'foo');
});

test("Capitalization should be significant", function() {
  assertNotEqual('foo', 'Foo');
});

test("Four-digit Unicode escapes should be possible with \\u", function() {
  assertEqual('n is n!', '\u006e is n!');
});

test("Two-hexidecimal-digit character escapes should be possible with \\x", function() {
  assertEqual("\x12\x34\x56", "\u0012\u0034\u0056");
});

test("A backslash should be able to escape a backslash.", function() {
  assertEqual("\x5C", "\\");
});

test("A backslash should be able to escape a double-quote in a double-quoted string.", function() {
  assertEqual('"', "\"");
});

test("A backslash should be able to escape a double-quote in a single-quoted string", function() {
  assertEqual('"', '\"');
});

test("A backslash should be able to escape a single-quote in a single-quoted string", function() {
  assertEqual("'", '\'');
});

test("A backslash should be able to escape a single-quote in a double-quoted string", function() {
  assertEqual("'", "\'");
});

test("\\r should give a carriage return", function() {
  assertEqual("\x0D", "\r");
});

test("\\f should give a form feed", function() {
  assertEqual("\x0c", "\f");
});

test("\\v should give a vertical tab", function() {
  assertEqual("\x0b", "\v");
});

test("\\n should give a newline", function() {
  assertEqual("\x0a", "\n");
});

test("\\t should give a tab", function() {
  assertEqual("\x09", "\t");
});

test("\\b should give a backspace", function() {
  assertEqual("\x08", "\b");
});

test("\\0 should give a null character", function() {
  assertEqual("\x00", "\0");
});

test("Undefined escapes should give their letters", function() {
  assertEqual("haha", "\h\a\h\a");
});

runTests();
