return ((undefined == undefined) == true) &&
    ((0/0 == 0/0) == false) &&
    ((0/0 == 12) == false) &&
    ((12 == 0/0) == false) &&
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
    ((false == -1) == false);
