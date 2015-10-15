#include "unity.h"
#include "subsystems/datalink/Mocktelemetry.h"
#include "Mockmessages_testable.h"
#include "peripherals/Mocksdcard_spi.h"
#include "loggers/sdlogger_spi_direct.h"

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ "Line: " S_(__LINE__)

/* Usage example of the SD Logger:
 * pprz_msg_send_ALIVE(&pprzlog_tp.trans_tx, &sdlogger_spi.device, AC_ID, value);
 *
 * Implementation requirements:
 * The device functions should be coupled to sdlogger-specific functions.
 * These include:
 * check_free_space
 * put_byte
 * send_message
 * char_available
 * get_byte
 */

/* Actually defined in sdcard.c */
//struct SDCard sdcard1;

/* Actually defined in spi.c */
//struct spi_periph spi2;

/* Actually defined in pprzlog_transport.c */
//struct pprzlog_transport pprzlog_tp;

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

void testInitializeLoggerStruct(void) {
  TEST_ASSERT_EQUAL(TRUE, FALSE);
}
