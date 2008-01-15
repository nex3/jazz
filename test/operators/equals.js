return ((undefined == undefined) == true) &&
    ((NaN == NaN) == false) &&
    ((NaN == 12) == false) &&
    ((12 == NaN) == false) &&
    ((-0 == 0) == true) &&
    ((0 == -0) == true) &&
    ((0 == 0) == true) &&
    ((12 == 12) == true) &&
    ((-1 == -178) == false) &&
    ((true == true) == true) &&
    ((false == false) == true) &&
    ((false == true) == false) &&
    ((true == false) == false) &&
    ((true == 1) == true) &&
    ((1 == true) == true) &&
    ((true == 17) == false) &&
    ((false == 0) == true) &&
    ((0 == false) == true) &&
    ((false == -1) == false) &&
    ("12" == 12) &&
    ("-12" == -12) &&
    (12e3 == "12e3") &&
    ("1.8e2" == 1.8e2) &&
    (-Infinity == "-Infinity") &&
    ("1" == true) &&
    ("0" == false) &&
    ("foo" == "foo") &&
    ("foo" != "foobar") &&
    ("bar" != "foobar") &&
    (undefined != "blah") &&

    ((undefined != undefined) == false) &&
    ((NaN != NaN) == true) &&
    ((NaN != 12) == true) &&
    ((12 != NaN) == true) &&
    ((-0 != 0) == false) &&
    ((0 != -0) == false) &&
    ((0 != 0) == false) &&
    ((12 != 12) == false) &&
    ((-1 != -178) == true) &&
    ((true != true) == false) &&
    ((false != false) == false) &&
    ((false != true) == true) &&
    ((true != false) == true) &&
    ((true != 1) == false) &&
    ((1 != true) == false) &&
    ((true != 17) == true) &&
    ((false != 0) == false) &&
    ((0 != false) == false) &&
    ((false != -1) == true);
