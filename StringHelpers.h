#pragma once
#include <Arduino.h>

// Returns a pointer to command's internal C-string buffer. The pointer is only valid as
// long as `command` itself lives and isn't modified/reassigned - use it immediately
// (e.g. passed straight into a display.print()/getStrWidth() call) rather than storing it.
char* string2char(String command);
