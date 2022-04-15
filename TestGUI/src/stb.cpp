#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// @NOTE: __STDC_LIB_EXT1__ is defined so that stb_image_write.h uses sprintf_s(). Otherwise, a compile error is thrown
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define __STDC_LIB_EXT1__
#include <stb/stb_image_write.h>

//
// NOTE: You must right click and build this once in order to activate stb_image
//
