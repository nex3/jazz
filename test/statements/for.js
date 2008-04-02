var res = true;

var i = -10, a = 0;
for (i = 0; i < 10; i++) a += 2;
res = (a == 20) && res;

i = 0, a = 0;
for (; i < 10; i++) a += 3;
res = (a == 30) && res;

i = 0, a = 0;
for (;i < 10;) {
  a += 4;
  i += 1;
}
res = (a == 40) && res;

for (;false;) return false;

a = 0;
for (var j = 0; j < 10; j++) a += 5;
res = (a == 50) && res;

a = 0;
for (;;) {
  if (a > 10)
  return res;
  else a++;
}
return false;
