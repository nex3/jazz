var res = true;

var a;
if (false) a = -1;
else a = 1;
res = (a == 1) && res;

if (12 == 1) a = -1;
else if (12 == 12) a = 1;
else if (12 == -12) a = -2;
else if (true) a = -3;
else a = 100;
res = (a == 1) && res;

if (1 == 1)
    if (1 == -1) a = 15;
    else a = 12;
res = (a == 12) && res;

if (1 == -1)
    if (1 == 12) a = 1;
    else a = 2;
else a = 3;
res = (a == 3) && res;

if (true) {
    var b;
    b = 17;
}
res = (b == 17) && res;

if (a = 1);
else if (a = -1);
res = (a == 1) && res;

if (1) a = 19;
res = (a == 19) && res;

return res;
