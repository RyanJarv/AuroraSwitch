#pragma once

// Local composite installer layout. All addresses are in the application area;
// calibration/settings below 0x90040000 and the official loader are untouched.
#define SELECTOR_RESIDENT_BASE 0x90040000
#define SELECTOR_RESIDENT_ENTRY 0x90058001
#define SELECTOR_RESIDENT_METADATA 0x90058100
#define SELECTOR_RESIDENT_IMAGE 0x90059000
#define SELECTOR_RESIDENT_CAPACITY 0x78000
