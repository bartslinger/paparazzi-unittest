#include "unity.h"
#include "subsystems/datalink/Mocktelemetry.h"
#include "Mockmessages_testable.h"
#include "peripherals/Mocksdcard_spi.h"
#include "loggers/sdlogger_spi_direct.h"

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ "Line: " S_(__LINE__)

/**
 * Usage example of the SD Logger:
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
//struct sdlogger_spi_periph sdlogger_spi;

/* Struct to save original state to revert to before each test */
struct sdlogger_spi_periph sdlogger_spi_original;

void setUp(void)
{
  /* Remember initial state */
  sdlogger_spi_original = sdlogger_spi;

  /* Set incorrect values to ensure proper initialization */
  //--

  Mocksdcard_spi_Init();
}

void tearDown(void)
{
  /* Revert back to original state */
  sdlogger_spi = sdlogger_spi_original;

  Mocksdcard_spi_Verify();
  Mocksdcard_spi_Destroy();
}

/**
 * @brief testInitializeLoggerStruct
 *
 * The device interface functions should be initialized to functions that are
 * defined by the SD Logger.
 * First argument of each function is a pointer to the peripheral
 */
void testInitializeLoggerStruct(void) {
  sdlogger_spi_direct_init();
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.check_free_space,
                        &sdlogger_spi_direct_check_free_space);
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.put_byte,
                        &sdlogger_spi_direct_put_byte);
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.send_message,
                        &sdlogger_spi_direct_send_message);
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.char_available,
                        &sdlogger_spi_direct_char_available);
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.get_byte,
                        &sdlogger_spi_direct_get_byte);
  TEST_ASSERT_EQUAL_PTR(sdlogger_spi.device.periph,
                        &sdlogger_spi);
}
