#include "std.h"
#include "unity.h"

#include "filters/heli_rate_filter.h"

/*
 * This is the feedback loop filter in the INDI rate loop. It contains a first
 * order filter and a delay, which can both be selected at run-time.
 *
 * omega_c is the bandwidth of the low-pass filter in rad/s. Smallest value is
 * 1 rad/s, which is increadibly really slow for fast dynamic systems.
 * delay is the number of timesteps delay in the output signal. Maximum delay is
 * 32 samples (also size of the buffer).
 *
 * Usage:
 * struct heli_rate_filter myfilter;
 *
 * uint32_t omega_c;
 * uint8_t delay;
 * int32_t filter_output;
 * output = heli_rate_filter_propagate(&myfilter, omega_c, delay);
 */

struct heli_rate_filter_t heli_roll_filter;

void setUp(void) {
  /* Initialize the struct with wrong values to ensure correct initialization */
  heli_roll_filter.omega = 123;
  heli_roll_filter.delay = 123;
  heli_roll_filter.alpha = 123;
  heli_roll_filter.previous = 123;
}

void tearDown(void) {

}

/**
 * @brief testInitializeFilterWithOmegaAndDelay
 * The value alpha only needs to be calculated once. This is done in
 * initialization (and on parameter update).
 * It should be initialized to the values given in the parameters.
 */
void testInitializeFilterWithOmegaAndDelay(void) {
  uint32_t omega = 20;
  uint8_t delay = 17;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay);
  TEST_ASSERT_EQUAL(20, heli_roll_filter.omega);
  TEST_ASSERT_EQUAL(17, heli_roll_filter.delay);
}

/**
 * @brief testStepResponseScenario1
 * Scenario: Delay = 0
 * omega_c = 20 (rad/s)
 * y[n] = alpha*y[n-1] + (1-alpha) u[n]
 * where alpha = 1 / ( 1 + omega_c * Ts )
 * Ts = 1/512, omega_c = 20, then alpha = 0.962406015
 *
 * First response is only (1-alpha)*9600, which is 360,9
 * Values used for comparison originate from matlab
 */
void testStepResponseScenario1(void) {
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay);

  /* Propagate */
  int32_t response;
  int32_t input = MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(360, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(707, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1041, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1362, response);
}

/**
 * @brief testStepResponseScenario2
 * Same as scenario1, but different omega.
 * Values originate from matlab script.
 */
void testStepResponseScenario2(void) {
  /* Configure filter */
  uint32_t omega = 60;
  uint8_t delay = 0;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay);

  /* Propagate */
  int32_t response;
  int32_t input = MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1007, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1908, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(2715, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(3437, response);
}

/**
 * @brief testStepResponseScenario3
 * Scenario WITH delay, 2 steps
 * Therefore, first two responses are zero.
 * Rest is the same as before.
 */
void testStepResponseScenario3(void){
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 2;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay);

  /* Propagate */
  int32_t response;
  int32_t input = MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(0, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(0, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(360, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(707, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1041, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1362, response);
}
