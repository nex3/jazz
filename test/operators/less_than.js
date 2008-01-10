return ((NaN < NaN) == false) &&
    ((NaN < 12) == false) &&
    ((12 < NaN) == false) &&
    ((-0 < 0) == false) &&
    ((0 < -0) == false) &&
    ((12 < 12) == false) &&
    ((0 < 0) == false) &&
    ((Infinity < 12) == false) &&
    ((Infinity < Infinity) == false) &&
    ((12 < Infinity) == true) &&
    ((12 < -Infinity) == false) &&
    ((-Infinity < 12) == true) &&
    ((-Infinity < Infinity) == true) &&
    ((Infinity < -Infinity) == false) &&
    ((1 < 2) == true) &&
    ((2 < 1) == false) &&

    ((NaN <= NaN) == false) &&
    ((NaN <= 12) == false) &&
    ((12 <= NaN) == false) &&
    ((-0 <= 0) == true) &&
    ((0 <= -0) == true) &&
    ((12 <= 12) == true) &&
    ((0 <= 0) == true) &&
    ((Infinity <= 12) == false) &&
    ((Infinity <= Infinity) == true) &&
    ((12 <= Infinity) == true) &&
    ((12 <= -Infinity) == false) &&
    ((-Infinity <= 12) == true) &&
    ((-Infinity <= Infinity) == true) &&
    ((Infinity <= -Infinity) == false) &&
    ((1 <= 2) == true) &&
    ((2 <= 1) == false);
