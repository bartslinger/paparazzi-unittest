#include "unity.h"
#include "subsystems/datalink/Mocktelemetry.h"
#include "Mockmessages_testable.h"
#include "peripherals/Mocksdcard_spi.h"
#include "subsystems/Mockimu.h"
#include "subsystems/actuators/Mockactuators_pwm_arch.h"
#include "loggers/sdlogger_spi_direct.h"

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ "Line: " S_(__LINE__)

/* Actually defined in sdcard.c */
struct SDCard sdcard1;

/* Actually defined in spi.c */
struct spi_periph spi2;

/* Actually defined in imu.c */
struct Imu imu;

/* Actually defined in actuators_pwm_arch.c */
int32_t actuators_pwm_values[ACTUATORS_PWM_NB];

/* Actually defined in pprz_transport.c */
struct pprz_transport pprz_tp;

/* Actually defined in sdlogger_spi_direct.c */
struct sdlogger_spi_periph sdlogger;


void setUp(void)
{
  /* Set incorrect values to ensure proper initialization */

  Mocksdcard_spi_Init();
}

void tearDown(void)
{
  Mocksdcard_spi_Verify();
  Mocksdcard_spi_Destroy();
}
