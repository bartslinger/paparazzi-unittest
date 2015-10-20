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
 * pprz_msg_send_ALIVE(&pprzlog_tp.trans_tx, &sdlogger_spi.device, AC_ID,
 * value);
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

/* Actually defined in radio_control.c */
struct RadioControl radio_control;

/* Actually defined in pprzlog_transport.c */
//struct pprzlog_transport pprzlog_tp;

/* Actually defined in sdlogger_spi_direct.c */
//struct sdlogger_spi_periph sdlogger_spi;

/* Struct to save original state to revert to before each test */
struct sdlogger_spi_periph sdlogger_spi_original;

#ifdef LOGGER_LED
/* Fake LED to test gpio calls */
bool_t FAKE_LED[3];

void gpio_set(uint32_t gpioport, uint16_t gpios)
{
  if (gpioport == LED_1_GPIO && gpios == LED_1_GPIO_PIN)
    FAKE_LED[0] = FALSE;
  if (gpioport == LED_2_GPIO && gpios == LED_2_GPIO_PIN)
    FAKE_LED[1] = FALSE;
  if (gpioport == LED_3_GPIO && gpios == LED_3_GPIO_PIN)
    FAKE_LED[2] = FALSE;
}

void gpio_clear(uint32_t gpioport, uint16_t gpios)
{
  if (gpioport == LED_1_GPIO && gpios == LED_1_GPIO_PIN)
    FAKE_LED[0] = TRUE;
  if (gpioport == LED_2_GPIO && gpios == LED_2_GPIO_PIN)
    FAKE_LED[1] = TRUE;
  if (gpioport == LED_3_GPIO && gpios == LED_3_GPIO_PIN)
    FAKE_LED[2] = TRUE;
}
#else
#warning Not testing LED functionality
#endif

bool_t LED_STATUS(uint8_t led) {
  return FAKE_LED[led-1];
}

void setUp(void)
{
  /* Remember initial state */
  sdlogger_spi_original = sdlogger_spi;

#ifdef LOGGER_LED
  /* Reset LED MOCK state */
  for (uint8_t i = 0; i < sizeof(FAKE_LED);i++) {
    FAKE_LED[i] = FALSE;
  }
#endif

  /* Set incorrect values to ensure proper initialization */
  sdlogger_spi.next_available_address = 123;
  sdlogger_spi.last_completed = 123;
  sdlogger_spi.accepting_messages = TRUE;
  sdlogger_spi.sdcard_buf_idx = 123;
  sdlogger_spi.switch_state = TRUE;

  /* Set incorrect values to sdcard buffers */
  for (uint16_t i = 0; i < SD_BLOCK_SIZE + 10; i++) {
    sdcard1.input_buf[i] = 123;
    sdcard1.output_buf[i] = 123;
  }
  /* Set incorrect values to logger buffer */
  for (uint8_t i = 0; i < sizeof(sdlogger_spi.buffer); i++) {
    sdlogger_spi.buffer[i] = 123;
  }

  Mocksdcard_spi_Init();
}

void tearDown(void)
{
  /* Revert back to original state */
  sdlogger_spi = sdlogger_spi_original;

  Mocksdcard_spi_Verify();
  Mocksdcard_spi_Destroy();
}

void helperInitializeLogger(void)
{
  /* Expectations */
  /* Expect the SD Card to be initialized in the call as well */
  sdcard_spi_init_Expect(&sdcard1,
                         &(SDLOGGER_SPI_LINK_DEVICE),
                         SDLOGGER_SPI_LINK_SLAVE_NUMBER);

  /* Call the function */
  sdlogger_spi_direct_init();
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
  helperInitializeLogger();

  /* Values in the struct should be set to their defaults */
  TEST_ASSERT_EQUAL(0x00000000, sdlogger_spi.next_available_address);
  TEST_ASSERT_EQUAL(0, sdlogger_spi.last_completed);
  TEST_ASSERT_FALSE(sdlogger_spi.accepting_messages);
  /* Initialize SD Card buffer at 1 because byte 0 is reserved for start flag */
  TEST_ASSERT_EQUAL(1, sdlogger_spi.sdcard_buf_idx);
  TEST_ASSERT_FALSE(sdlogger_spi.switch_state);
  for (uint8_t i = 0; i < sizeof(sdlogger_spi.buffer); i++) {
    TEST_ASSERT_EQUAL(0, sdlogger_spi.buffer[i]);
  }
  /*  Link device function references: */
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

/**
 * @brief testReadIndexWhenSDCardGetsIdle
 * As soon as the periodic functions get called, the SD Card runs the
 * initialization sequence. When it is done with this, the logger should request
 * the index, which is located at the beginning of the second 4MiB block, at
 * address 0x2000.
 */
void testReadIndexWhenSDCardGetsIdle(void)
{
  /* Preconditions */
  helperInitializeLogger();
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
 * @brief testReadIndexOnlyOnceOnStartup
 * We don't want to read the index every time the SD Card is idle. We can check
 * if it is done already by checking the next_available_address. This is zero by
 * default.
 */
void testReadIndexOnlyOnceOnStartup(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdcard1.status = SDCard_Idle;
  /* Not zero, so the index was read already: */
  sdlogger_spi.next_available_address = 0x00004000;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* read_block is not called this time */

  /* Periodic loop */
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

  /* Verify correct values set by function */
  TEST_ASSERT_EQUAL(0x12345678, sdlogger_spi.next_available_address);
  TEST_ASSERT_EQUAL(12, sdlogger_spi.last_completed);
}

/**
 * @brief testStartLoggingWhenSwitchIsFlipped
 * The log procedure is started if the switch is flipped.
 * Value of the switch ranges from -MAX_PPRZ to MAX_PPRZ. It is assumed that
 * values above 0 is ON, below 0 is OFF.
 */
void testStartLoggingWhenSwitchIsFlipped(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Index already available: */
  sdlogger_spi.next_available_address = 0x00004000;
  sdlogger_spi.accepting_messages = FALSE;
  /* Switch is now state OFF: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = -500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* First Periodic loop */
  sdlogger_spi_direct_periodic();

  /* Switch turned ON: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;

  /* Expections second run */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_start_Expect(&sdcard1, 0x00004000);

  /* Second periodic loop */
  sdlogger_spi_direct_periodic();

  /* Logger is accepting messages */
  TEST_ASSERT_TRUE(sdlogger_spi.accepting_messages);
  /* LED is on */
#ifdef LOGGER_LED
  TEST_ASSERT_TRUE(LED_STATUS(LOGGER_LED));
#endif
}

void testOnlyStartMultiwriteOnceWhenSwitchIsFlipped(void) {
  /* Preconditions */
  helperInitializeLogger();
  /* Index already available: */
  sdlogger_spi.next_available_address = 0x00004000;
  sdlogger_spi.accepting_messages = TRUE;
  /* Switch is now state ON: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_start_Expect(&sdcard1, 0x00004000);

  /* First Periodic loop */
  sdlogger_spi_direct_periodic();

  /* Expections second run */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* Do not expect multiwrite_start again */

  /* Second periodic loop */
  sdlogger_spi_direct_periodic();
}

/**
 * @brief testDoNotStartLoggingIfIndexNotDownloaded
 * Only start accepting messages if the index frame has been downloaded. Then,
 * the next available address is also set correctly.
 */
void testDoNotStartLoggingIfIndexNotDownloaded(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Index not yet available: */
  sdlogger_spi.next_available_address = 0x00000000;
  sdlogger_spi.accepting_messages = FALSE;
  /* Switch in state ON: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_read_block_Expect(&sdcard1,
                               0x00002000,
                               &sdlogger_spi_direct_index_received);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  /* Logger is NOT accepting messages */
  TEST_ASSERT_FALSE(sdlogger_spi.accepting_messages);
}

/**
 * @brief testDoNotLogIfSwitchIsDisabled
 * No logging if switch is off.
 */
void testDoNotLogIfSwitchIsDisabled(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Index not yet available: */
  sdlogger_spi.next_available_address = 0x00004000;
  sdlogger_spi.accepting_messages = FALSE;
  /* Switch in state OFF: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = -500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  /* Logger is NOT accepting messages */
  TEST_ASSERT_FALSE(sdlogger_spi.accepting_messages);
}

/**
 * @brief testCheckFreeSpaceNotLogging
 * If not accepting messages, return FALSE to prevent further calls to write.
 */
void testCheckFreeSpaceNotLogging(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Not accepting messages */
  sdlogger_spi.accepting_messages = FALSE;

  /* Function call (from messages.h) */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       20);

  /* No space available for writing */
  TEST_ASSERT_FALSE(available);
}

/**
 * @brief testCheckFreeSpaceLoggingAndAvailable
 * State of the logger is accepting messages. Return TRUE.
 */
void testCheckFreeSpaceLoggingAndAvailable(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Accepting messages */
  sdlogger_spi.accepting_messages = TRUE;

  /* Function call (from messages.h) */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       20);

  /* There is space available for writing */
  TEST_ASSERT_TRUE(available)
}

/**
 * @brief testPutByteWhileNoSpaceLeft
 * No tests are done inside the logger to check if there is still space left
 * when writing a byte. This check should be done outside the logger. It is
 * normally checked in messages.h, by calling check_free_space.
 */
void testPutByteWhileNoSpaceLeft(void)
{
  /* Empty on purpose */
}

/**
 * @brief testPutByteIntoSDBuffer
 * Put bytes in the SD Card buffer directly
 */
void testPutByteIntoSDBuffer(void)
{
  /* Preconditions */
  helperInitializeLogger();

  /* Put byte call through messages.h and pprzlog_tp */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xAB);
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xEF);
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0x33);

  /* Check if the byte was successfully stuffed in SD Card output buffer */
  TEST_ASSERT_EQUAL(0xAB, sdcard1.output_buf[1]);
  TEST_ASSERT_EQUAL(0xEF, sdcard1.output_buf[2]);
  TEST_ASSERT_EQUAL(0x33, sdcard1.output_buf[3]);
}

/**
 * @brief testSDBufferGetsFull
 * If the SD Card buffer is full, it will be busy writing this stuff. Until the
 * SPI transaction is finished, any further write requests are saved in the
 * logger buffer. If then the SPI transaction is finished, this logger buffer is
 * copied into the SD Card buffer.
 */
void testSDBufferGetsFull(void)
{
  /* Pre-conditions */
  helperInitializeLogger();
  /* Last element in the buffer: */
  sdlogger_spi.sdcard_buf_idx = 512;

  /* Expectations */
  sdcard_spi_multiwrite_next_Expect(&sdcard1);

  /* Put byte call through messages.h and pprzlog_tp */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xAB);

  /* Values set in sd buffer, index reset */
  TEST_ASSERT_EQUAL(0xAB, sdcard1.output_buf[512]);
  TEST_ASSERT_EQUAL(1, sdlogger_spi.sdcard_buf_idx);

  /* Write another byte after SD Card buffer is full */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xEF);

  /* It should end up in the loggers internal buffer to copy later */
  TEST_ASSERT_EQUAL(0xEF, sdlogger_spi.buffer[0]);

}

void testStopLogging(void)
{
  TEST_IGNORE();
}

