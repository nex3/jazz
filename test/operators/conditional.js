var a = 1;
return ((true ? 1 : 2) == 1) &&
    ((false ? 1 : 2) == 2) &&
    ((true ? false ? 1 : 2 : 3) == 2) &&
    ((false ? 1 : true ? 2 : 3) == 2) &&
    ((true ? 1 : a = 2, a) == 1) &&
    ((false ? a = 2 : 1, a) == 1) &&
    ((true ? a = 2 : a = 3) == 2) &&
    (("foo" ? 1 : 2) == 1) &&
    (("" ? 1 : 2) == 2);
