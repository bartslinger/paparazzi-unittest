#include "std.h"
#include "unity.h"

#include "filters/second_order_delayed_filter.h"

/*
 * Usage:
 * struct second_order_delayed_filter myfilter;
 *
 * output = heli_rate_filter_propagate(&myfilter, delay);
 */

struct SecondOrderDelayedFilter myfilter;

void setUp(void) {
  /* Initialize the struct with wrong values to ensure correct initialization */
  myfilter.x[0] = 123;
  myfilter.x[1] = 123;

  /* Set values to be used as in the matlab study */
  myfilter.A[0] = 13969;
  myfilter.A[1] = -1480;
  myfilter.A[2] = 947;
  myfilter.A[3] = 16337;

  myfilter.B[0] = 23676;
  myfilter.B[1] = 760;

  myfilter.C[0] = 0;
  myfilter.C[1] = 1;

  myfilter.D[0] = 0;

}

void tearDown(void) {

}

void testInitialize(void)
{
  // Check state reset
  second_order_delayed_filter_initialize(&myfilter);
  TEST_ASSERT_EQUAL(0, myfilter.x[0]);
  TEST_ASSERT_EQUAL(0, myfilter.x[1]);
}

/**
 * @brief testStepInputResponse
 * According to matlab simulation
 */
void testStepInputResponse(void)
{
  second_order_delayed_filter_initialize(&myfilter);
  int16_t sigout = 0;

  int16_t inputs[10] = {0, 9600, 9600, 9600, 9600,
                        9600, 9600, 9600, 9600, 9600};
  int16_t outputs[10] ={0, 0, 27, 104, 223,
                        377, 561, 769, 997, 1240};
  for (int i = 0; i < 10; i++) {
    sigout = second_order_delayed_filter_propagate(&myfilter, inputs[i]);
    TEST_ASSERT_EQUAL(outputs[i], sigout);
  }

}
