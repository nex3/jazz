({}).prototype.foo = 12;
var a = {};
a.foo = 15;
var b = {};
return (({}).foo == 12 &&
        a.foo == 15 &&
        a.prototype.foo == 12 &&
        b.foo == 12 &&
        b.prototype == ({}).prototype);
