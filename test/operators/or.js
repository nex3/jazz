return ((true || true) == true) &&
  ((true || false) == true) &&
  ((false || true) == true) &&
  ((false || false) == false) &&
  ((1 || 2) == 1) &&
  ((false || 1) == 1) &&
  ((false || false || true) == true) &&
  ((false || true || false) == true) &&
  ((true || false || false) == true) &&
  ((true || true && false) == true);
