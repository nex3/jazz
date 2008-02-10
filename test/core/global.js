return isNaN(NaN) && isNaN("foobar") &&
    !isNaN("12") && !isNaN(false) && isNaN();
