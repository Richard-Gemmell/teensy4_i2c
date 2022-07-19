This project does not follow the standard layout for PlatformIO tests.
I've modified so it can run lots of different test suits in many directories.

This directory contains the actual tests; both unit and full stack (e2e) tests. 
They're executed by `test/test_runner.cpp`.

All tests must extend the `TestSuite` class. See `example/example.h` for an
example of a simple test.

More information about PIO Unit Testing:
- https://docs.platformio.org/page/plus/unit-testing.html
