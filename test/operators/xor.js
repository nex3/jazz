return ((1 ^ 2) == 3) &&
    ((9 ^ 3) == 10) &&
    ((-5 ^ 7) == -4) &&
    ((2147483650 ^ 2) == -2147483648) &&
    ((34359738385 ^ 2) == 19) &&
    ((9 ^ 3 | 5 ^ 4) == 11) &&
    ((1 ^ 2 & 5 ^ 4) == 5);