var a = 1 +/*2;*/1;
var b = /* Foo
Bar */ 4;
var c = /* "*/"foo*/";
return (/**/a/**/ == 2 &&
        b == 4 &&
        c == "foo*/");/*
Foo */