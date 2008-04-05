load("test/core/_to_load.js");
return foo == "bar" && isNaN(NaN) && isNaN("foobar") &&
  !isNaN("12") && !isNaN(false) && isNaN();
