var res = true;

var a;
switch (a = 10) {}
res = (a == 10) && res;

switch (12) {
case -12: return false;
case 12: a = 2;
case false: a *= 3;
default:
case -1: a += 1;
case undefined: a *= 2;
}
res = (a == 14) && res;

switch (0) {
case false: return false;
case undefined: return false;
case 0: a = 11;
case -1: a += 2;
}
res = (a == 13) && res;

switch (-15) {
default: return false;
case 15: return false;
case -15: a = -5;
case 2:
case 3: a *= 2;
}
res = (a == -10) && res;

switch (12) {
default: a = 115;
case 1: a /= 5;
case -1: a += 2;
}
res = (a == 25) && res;

switch (12) {
case 1: a = 0;
default: a = 2;
}
res = (a == 2) && res;

switch (12) {
default: a = 3;
}
res = (a == 3) && res;

return res;
