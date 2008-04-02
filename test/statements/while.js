var res = true;

var a = 0;
do a++; while (a < 10);
res = (a == 10) && res;

a = 0;
do a++; while (0);
res = (a == 1) && res;

var a = 0, i = 0;
do {
  a += 3;
  i += 1;
} while (i < 10);
res = (a == 30) && res;


var a = 0;
while (a < 10) a++;
res = (a == 10) && res;

a = 0;
while (0) a = 17;
res = (a == 0) && res;

var a = 0, i = 0;
while (i < 10) {
  a += 3;
  i += 1;
}
res = (a == 30) && res;

return res;
