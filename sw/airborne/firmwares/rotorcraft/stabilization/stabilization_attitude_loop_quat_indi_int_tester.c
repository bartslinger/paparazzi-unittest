#include "std.h"
#include "unity.h"

#include "stabilization/stabilization_attitude_loop_quat_indi_int.h"


int32_t stabilization_att_indi_cmd[COMMANDS_NB];

struct Int32AttitudeGains stabilization_gains = {
  {STABILIZATION_ATTITUDE_PHI_PGAIN, STABILIZATION_ATTITUDE_THETA_PGAIN, STABILIZATION_ATTITUDE_PSI_PGAIN },
  {STABILIZATION_ATTITUDE_PHI_DGAIN, STABILIZATION_ATTITUDE_THETA_DGAIN, STABILIZATION_ATTITUDE_PSI_DGAIN },
  {STABILIZATION_ATTITUDE_PHI_DDGAIN, STABILIZATION_ATTITUDE_THETA_DDGAIN, STABILIZATION_ATTITUDE_PSI_DDGAIN },
  {STABILIZATION_ATTITUDE_PHI_IGAIN, STABILIZATION_ATTITUDE_THETA_IGAIN, STABILIZATION_ATTITUDE_PSI_IGAIN }
};

struct Int32Quat att_err;
struct Int32Rates actual_body_rate;
struct Int32Rates *body_rate;

void setUp(void) {
  /* Initialize the struct with wrong values to ensure correct initialization */
  body_rate = &actual_body_rate;

  stabilization_att_indi_cmd[COMMAND_ROLL] = 123;
}

void tearDown(void) {

}

void testINDIpropagate(void)
{
  /* Run stabilization */

  attitude_run_indi(stabilization_att_indi_cmd,
                    &stabilization_gains,
                    &att_err,
                    (*body_rate));
  TEST_ASSERT_EQUAL(354, stabilization_att_indi_cmd[COMMAND_ROLL]);
}

