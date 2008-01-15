return ('foo' == "foo") &&
    ('Foo' != 'foo') &&
    ('Snowma\u006e: \u2603!' == 'Snowman: ☃!') &&
    ("\x12\x34\x56" == "\u0012\u0034\u0056") &&
    ("Back\\slash" == "Back\x5Cslash") &&
    ("Double\"quote" == 'Double"quote') &&
    ('Double\"quote' == 'Double\x22quote') &&
    ("Single\'quote" == "Single'quote") &&
    ('Single\'quote' == 'Single\x27quote') &&
    ('\r' == '\x0D') &&
    ('\f' == '\x0c') &&
    ('\v' == '\x0b') &&
    ('\n' == '\x0a') &&
    ('\t' == '\x09') &&
    ('\b' == '\x08') &&
    ('\0' == '\x00') &&
    ("\h\a\h\a" == "haha");
    
