var a = "hello";
this.b = "hello";
var c = function() {};
return ((function() { return 12; })() == 12 &&
        (function() { var a = 17; return a + 1; })() == 18 &&
        a == "hello" &&
        (function() { b = "goodbye"; })() == undefined &&
        b == "goodbye" &&
        c == c &&
        c != function() {1;} &&
        c() == undefined &&
        function() { b = "Woop!"; } &&
        b == "goodbye");
