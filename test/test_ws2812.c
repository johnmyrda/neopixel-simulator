#include "unity.h"
#include "ws2812.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_pixel_to_hex(void) {
    char hexval[] = "0000000000";
    rgb_pixel_t * pixel = malloc(sizeof(rgb_pixel_t));
    pixel->red = 255;
    pixel->green = 0;
    pixel->blue = 255;
    ws2812_pixel_to_hex(pixel, hexval);
    TEST_ASSERT_EQUAL_STRING("FF00FF", &hexval);
    free(pixel);
}

void test_a_different_thing(void) {
    //more test stuff
}

// // not needed when using generate_test_runner.rb
// int main(void) {
//     UNITY_BEGIN();
//     RUN_TEST(test_pixel_to_hex);
//     RUN_TEST(test_function_should_doAlsoDoBlah);
//     return UNITY_END();
// }