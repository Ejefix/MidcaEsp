#include <unity.h>

extern "C" void setUp()
{
}

extern "C" void tearDown()
{
}

void test_two_plus_two()
{
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_two_plus_two);

    return UNITY_END();
}