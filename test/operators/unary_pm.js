return (+15 == 15) &&
    (+(-12) == -12) &&
    (+true === 1) &&
    (+false === 0) &&
    (+undefined != +undefined) &&
    ((+"foo") + "" == "NaN") &&
    (+"12" == "12") &&
    (+" 1.3e6 \n " == 1.3e6) &&
    (+"\u205F -8 " == -8) &&
    (+"+12" == 12) &&
    (-(12) == -12) &&
    (-(-180) == 180) &&
    (-(0 - 15) == 15) &&
    (-true == -1);
