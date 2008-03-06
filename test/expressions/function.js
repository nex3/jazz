var a = "hello";
this.b = "hello";
var c = function() {};
var state = (function() { var b; return function(a) {
    return a == 0 ? function() { return b; } : function (a) { b = a; };
};})();
var get = state(0);
var set = state(1);
return ((function() { return 12; })() == 12 &&
        (function() { var a = 17; return a + 1; })() == 18 &&
        a == "hello" &&
        (function() { b = "goodbye"; })() == undefined &&
        b == "goodbye" &&
        c == c &&
        c != function() {1;} &&
        c() == undefined &&
        function() { b = "Woop!"; } &&
        b == "goodbye" &&
        ((function() { a = "blather"; })(), true) &&
        a == "blather" && 
        get() == undefined &&
        (set(12), get()) == 12 &&
        get() == 12 &&
        (set("Hello"), get()) == "Hello");
