#pragma once

#include <string>

//the test generates 10^TEST_KEY_LENGTH keys
#define TEST_KEY_LENGTH 1

#define VAL_STR "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"

typedef void (*_kv_f)(std::string key, std::string value);

void _kv_generator(std::string s, unsigned int chars_left, _kv_f pf);
void _kv_put(std::string key, std::string value);
void _kv_get(std::string key, std::string expected_value);
void _test_kv_put();
void _test_kv_get();
