#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#include <sky/data_descriptor.h>
#include <sky/sky_string.h>
#include <sky/mem.h>

#include "minunit.h"


//==============================================================================
//
// Fixtures
//
//==============================================================================

char INT_DATA[] = "\xD1\x03\xE8";

char DOUBLE_DATA[] = "\xCB\x40\x59\x0C\xCC\xCC\xCC\xCC\xCD";

char BOOLEAN_FALSE_DATA[] = "\xC2";

char BOOLEAN_TRUE_DATA[] = "\xC3";

char STRING_DATA[] = "\xa3\x66\x6f\x6f";


//==============================================================================
//
// Definitions
//
//==============================================================================

typedef struct {
    int64_t dummy;
    int64_t int_value;
    double double_value;
    bool boolean_value;
    sky_string string_value;
} test_t;


//==============================================================================
//
// Test Cases
//
//==============================================================================

//--------------------------------------
// Lifecycle
//--------------------------------------

int test_sky_data_descriptor_new() {
    sky_data_descriptor *descriptor = sky_data_descriptor_new(-20, 30);
    mu_assert_int_equals(descriptor->property_count, 115);
    mu_assert_int64_equals(descriptor->property_descriptors[0].property_id, -52);
    mu_assert_int64_equals(descriptor->property_zero_descriptor[0].property_id, 0);
    mu_assert_int64_equals(descriptor->property_zero_descriptor[30].property_id, 30);
    mu_assert_int64_equals(descriptor->property_zero_descriptor[62].property_id, 62);
    sky_data_descriptor_free(descriptor);
    return 0;
}


//--------------------------------------
// Property Management
//--------------------------------------

int test_sky_data_descriptor_set_integer() {
    test_t obj;
    memset(&obj, 0, sizeof(obj));
    size_t sz;
    sky_data_descriptor *descriptor = sky_data_descriptor_new(0, 1);
    sky_data_descriptor_set_property(descriptor, 1, offsetof(test_t, int_value), "integer");
    mu_assert_int_equals(descriptor->property_zero_descriptor[1].offset, 8);
    sky_data_descriptor_set_value(descriptor, (void*)(&obj), 1, INT_DATA, &sz);
    mu_assert_long_equals(sz, 3L);
    mu_assert_int64_equals(obj.int_value, 1000LL);
    sky_data_descriptor_free(descriptor);
    return 0;
}

int test_sky_data_descriptor_set_double() {
    test_t obj;
    size_t sz;
    sky_data_descriptor *descriptor = sky_data_descriptor_new(-1, 0);
    sky_data_descriptor_set_property(descriptor, -1, offsetof(test_t, double_value), "float");
    mu_assert_int_equals(descriptor->property_zero_descriptor[-1].offset, 16);
    sky_data_descriptor_set_value(descriptor, (void*)(&obj), -1, DOUBLE_DATA, &sz);
    mu_assert_long_equals(sz, 9L);
    mu_assert_bool(fabs(obj.double_value - 100.2) < 0.1);
    sky_data_descriptor_free(descriptor);
    return 0;
}

int test_sky_data_descriptor_set_boolean() {
    test_t obj;
    size_t sz;
    sky_data_descriptor *descriptor = sky_data_descriptor_new(0, 2);
    sky_data_descriptor_set_property(descriptor, 2, offsetof(test_t, boolean_value), "boolean");
    mu_assert_int_equals(descriptor->property_zero_descriptor[2].offset, 24);
    sky_data_descriptor_set_value(descriptor, (void*)(&obj), 2, BOOLEAN_TRUE_DATA, &sz);
    mu_assert_long_equals(sz, 1L);
    mu_assert_bool(obj.boolean_value == true);
    sky_data_descriptor_set_value(descriptor, (void*)(&obj), 2, BOOLEAN_FALSE_DATA, &sz);
    mu_assert_long_equals(sz, 1L);
    mu_assert_bool(obj.boolean_value == false);
    sky_data_descriptor_free(descriptor);
    return 0;
}

int test_sky_data_descriptor_set_string() {
    test_t obj;
    size_t sz;
    sky_data_descriptor *descriptor = sky_data_descriptor_new(0, 1);
    sky_data_descriptor_set_property(descriptor, 1, offsetof(test_t, string_value), "string");
    mu_assert_int_equals(descriptor->property_zero_descriptor[1].offset, 32);
    sky_data_descriptor_set_value(descriptor, (void*)(&obj), 1, STRING_DATA, &sz);
    mu_assert_long_equals(sz, 4L);
    mu_assert_int_equals(obj.string_value.length, 3);
    mu_assert_bool(obj.string_value.data == &STRING_DATA[1]);
    sky_data_descriptor_free(descriptor);
    return 0;
}


//==============================================================================
//
// Setup
//
//==============================================================================

int all_tests() {
    mu_run_test(test_sky_data_descriptor_new);
    mu_run_test(test_sky_data_descriptor_set_integer);
    mu_run_test(test_sky_data_descriptor_set_double);
    mu_run_test(test_sky_data_descriptor_set_boolean);
    mu_run_test(test_sky_data_descriptor_set_string);
    return 0;
}

RUN_TESTS()