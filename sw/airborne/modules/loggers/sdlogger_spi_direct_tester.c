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
 *
 * In case of pprzlog_tp, a call to pprz_msg_send_*** results in the following
 * call sequence:
 * - check_free_space
 * - put_byte (numerous times depending on message)
 * - send_message
 *
 * Put file index at address 0x2000
 * Start of logdata at address 0x4000
 */

/* Actually defined in sdcard.c */
struct SDCard sdcard1;

/* Actually defined in spi.c */
struct spi_periph spi2;

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

void helperAssignUint(uint8_t location[], uint32_t value) {
  location[0] = value >> 24;
  location[1] = value >> 16;
  location[2] = value >> 8;
  location[3] = value >> 0;
}

/**
 * @brief testHelperAssignUint
 * Function to make sure the helper does the right thing
 */
void testHelperAssignUint(void) {
  helperAssignUint(&sdcard1.input_buf[32], 0x12345678);
  TEST_ASSERT_EQUAL(0x12, sdcard1.input_buf[32]);
  TEST_ASSERT_EQUAL(0x34, sdcard1.input_buf[33]);
  TEST_ASSERT_EQUAL(0x56, sdcard1.input_buf[34]);
  TEST_ASSERT_EQUAL(0x78, sdcard1.input_buf[35]);
}

/**
 * @brief testInitializeLoggerStruct
 *
 * The device interface functions should be initialized to functions that are
 * defined by the SD Logger.
 * First argument of each function is a pointer to the peripheral
 */
void testInitializeLoggerStruct(void)
{
  /* Expect the SD Card to be initialized in the call as well */
  sdcard_spi_init_Expect(&sdcard1,
                         &(SD_LOGGER_SPI_LINK_DEVICE),
                         SD_LOGGER_SPI_LINK_SLAVE_NUMBER);

  /* Call the function */
  sdlogger_spi_direct_init();

  /* Test results */
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

void testPeriodicFunctionCallsSDCard(void)
{
  /* Call the SD Cards periodic function each periodic loop */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Do this in the logger periodic function */
  sdlogger_spi_direct_periodic();
}

/**
 * @brief testReadFirstBlockWhenSDCardGetsIdle
 * As soon as the periodic functions get called, the SD Card runs the
 * initialization sequence. When it is done with this, the logger should request
 * the index, which is located at the beginning of the second 4MiB block, at
 * address 0x2000.
 */
void testReadFirstBlockWhenSDCardGetsIdle(void)
{
  /* Preconditions */
  sdcard1.status = SDCard_Idle;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_read_block_Expect(&sdcard1,
                               0x00002000,
                               &sdlogger_spi_direct_index_received);

  /* Function call */
  sdlogger_spi_direct_periodic();
}

/**
 * @brief testSaveIndexInformationForLaterUse
 * A callback is generated when the index page is received, upon request just
 * after initialization. The next available address and last completed log
 * information are stored for later use.
 */
void testSaveIndexInformationForLaterUse(void)
{
  /* Preconditions */
  /* SD Card buffer contains relevant values */
  /* Next available address: */
  helperAssignUint(&sdcard1.input_buf[0], 0x12345678);
  /* Last completed log: */
  sdcard1.input_buf[4] = 12;

  /* Simulated callback from SD Card read function */
  sdlogger_spi_direct_index_received();

  TEST_ASSERT_EQUAL(0x12345678, sdlogger_spi.next_available_address);

}
