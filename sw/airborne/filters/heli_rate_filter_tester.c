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
  for (uint8_t i=0; i<HELI_RATE_FILTER_BUFFER_SIZE;i++) {
    heli_roll_filter.buffer[i] = 123+i;
  }
  heli_roll_filter.idx = 123;
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
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);
  TEST_ASSERT_EQUAL(20, heli_roll_filter.omega);
  TEST_ASSERT_EQUAL(17, heli_roll_filter.delay);
  TEST_ASSERT_EQUAL(10000, heli_roll_filter.max_inc);
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
 *            0
 *          360
 *          707
 *         1041
 *         1362
 *         1671
 *         1969
 *         2255
 *         2531
 *         2796
 */
void testStepResponseScenario1(void) {
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

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
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

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
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

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

/**
 * @brief testStepResponseBufferOverflow
 * Same as before, but with different buffer index to see if it correctly jumps
 * back to the start of the buffer after max index is reached.
 */
void testStepResponseBufferOverflow(void){
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 2;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Manipulate buffer index */
  heli_roll_filter.idx = HELI_RATE_FILTER_BUFFER_SIZE - 2;

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

/**
 * @brief testMaximumDelayFirst
 * It should not be possible to set the delay to a value larger than the buffer
 * size. If this is attempted, it should be rounded off towards the buffer size
 * minus one.
 */
void testMaximumDelayFirst(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = HELI_RATE_FILTER_BUFFER_SIZE;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  TEST_ASSERT_EQUAL(HELI_RATE_FILTER_BUFFER_SIZE-1, heli_roll_filter.delay);
}

/**
 * @brief testMaximumDelayFar
 * Requested delay exceeds buffer size by far, is rounded towards buffer size
 * minus one.
 */
void testMaximumDelayFar(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = HELI_RATE_FILTER_BUFFER_SIZE + 5;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  TEST_ASSERT_EQUAL(HELI_RATE_FILTER_BUFFER_SIZE-1, heli_roll_filter.delay);
}

/**
 * @brief testChangeOmegaValue
 * During runtime, the value for omega can be changed using a function.
 * We initialize with 20, then change it to 60 to see the response.
 */
void testChangeOmegaValue(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Propagate with no input */
  int32_t response;
  response = heli_rate_filter_propagate(&heli_roll_filter, 0);

  /* Change omega to 60 rad/s */
  heli_rate_filter_set_omega(&heli_roll_filter, 60);

  /* Propagate */
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
 * @brief testChangeDelayWithinRange
 * Run-time change of delay.
 */
void testChangeDelayWithinRange(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Propagate */
  int32_t response;
  int32_t input = MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(360, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(707, response);

  /* Change delay from 0 to 2 */
  heli_rate_filter_set_delay(&heli_roll_filter, 2);
  /* Propagate, values are repeated */
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
 * @brief testChangeDelayOutsideBufferRange
 * Change delay during run-time. New delay value is bigger than buffer size. In
 * this case, set delay to buffersize - 1
 */
void testChangeDelayOutsideBufferRange(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 10000;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Set delay to number that is 1 too large */
  heli_rate_filter_set_delay(&heli_roll_filter, HELI_RATE_FILTER_BUFFER_SIZE);
  TEST_ASSERT_EQUAL(HELI_RATE_FILTER_BUFFER_SIZE-1, heli_roll_filter.delay);
}

/**
 * @brief testMaximumIncreaseRate
 * Because actuators are saturated, increase only by a maximum. Otherwise first
 * order.
 * Max rate in this test = 340. Third value (1015) is not saturated anymore.
 *            0
 *          340
 *          680
 *         1015
 *         1337
 *         1647
 *         1946
 *         2233
 *         2509
 *         2775
 */
void testMaximumIncreaseRate(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 340;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Propagate */
  int32_t response;
  int32_t input = MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(340, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(680, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1015, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(1337, response);
}

/**
 * @brief testNegativeSlopeMaximumRate
 * Same as previous test but with negative input and negative results
 *            0
 *         -340
 *         -680
 *        -1016
 *        -1339
 *        -1650
 *        -1949
 *        -2237
 *        -2514
 *        -2781
 */
void testNegativeSlopeMaximumRate(void)
{
  /* Configure filter */
  uint32_t omega = 20;
  uint8_t delay = 0;
  uint16_t max_inc = 340;
  heli_rate_filter_initialize(&heli_roll_filter, omega, delay, max_inc);

  /* Propagate */
  int32_t response;
  int32_t input = -MAX_PPRZ;
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(-340, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(-680, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(-1016, response);
  response = heli_rate_filter_propagate(&heli_roll_filter, input);
  TEST_ASSERT_EQUAL(-1339, response);
}
