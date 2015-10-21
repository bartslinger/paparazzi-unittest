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

bool_t LED_STATUS(uint8_t led) {
  return FAKE_LED[led-1];
}

void LED_SET(uint8_t led, bool_t status) {
  FAKE_LED[led-1] = status;
}
#else
#warning Not testing LED functionality
#endif


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
  sdlogger_spi.status = SDLogger_Error;
  sdlogger_spi.next_available_address = 123;
  sdlogger_spi.last_completed = 123;
  sdlogger_spi.sdcard_buf_idx = 123;
  sdlogger_spi.idx = 123;
  sdlogger_spi.log_len = 123;

  /* Set incorrect values to sdcard buffers */
  for (uint16_t i = 0; i < SD_BLOCK_SIZE + 10; i++) {
    sdcard1.input_buf[i] = 123;
    sdcard1.output_buf[i] = 123;
  }
  /* Set incorrect values to logger buffer */
  for (uint8_t i = 0; i < sizeof(sdlogger_spi.buffer); i++) {
    sdlogger_spi.buffer[i] = 123;
  }
  sdcard1.status = SDCard_Error;

  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 0;

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
  TEST_ASSERT_EQUAL(SDLogger_Initializing, sdlogger_spi.status);
  TEST_ASSERT_EQUAL(0x00000000, sdlogger_spi.next_available_address);
  TEST_ASSERT_EQUAL(0, sdlogger_spi.last_completed);
  /* Initialize SD Card buffer at 1 because byte 0 is reserved for start flag */
  TEST_ASSERT_EQUAL(1, sdlogger_spi.sdcard_buf_idx);
  for (uint8_t i = 0; i < sizeof(sdlogger_spi.buffer); i++) {
    TEST_ASSERT_EQUAL(0, sdlogger_spi.buffer[i]);
  }
  TEST_ASSERT_EQUAL(0, sdlogger_spi.idx);
  TEST_ASSERT_EQUAL(0, sdlogger_spi.log_len);
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
 * @brief testDoNothingWhileInitializing
 * Wait for the SD Card to become idle.
 */
void testDoNothingWhileInitializing(void)
{
  /* Preconditions */
  sdlogger_spi.status = SDLogger_Initializing;
  sdcard1.status = SDCard_Busy;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* Call no other functions */

  sdlogger_spi_direct_periodic();

  /* Still initializing state */
  TEST_ASSERT_EQUAL(SDLogger_Initializing, sdlogger_spi.status);
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
  sdlogger_spi.status = SDLogger_Initializing;
  sdcard1.status = SDCard_Idle;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_read_block_Expect(&sdcard1,
                               0x00002000,
                               &sdlogger_spi_direct_index_received);

  /* Function call */
  sdlogger_spi_direct_periodic();

  /* Status change */
  TEST_ASSERT_EQUAL(SDLogger_RetreivingIndex, sdlogger_spi.status);
}

void testWaitWhileRetreivingIndex(void)
{
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_RetreivingIndex;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* Expect no other calls */

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  /* State unchanged */
  TEST_ASSERT_EQUAL(SDLogger_RetreivingIndex, sdlogger_spi.status);
}

/**
 * @brief testSaveIndexInformationForLaterUse
 * A callback is generated when the index page is received, upon request just
 * after initialization. The next available address and last completed log
 * information are stored for later use.
 *
 * For now, this information is not used but overwritten. Multiple logs are
 * possible while logging with power on.
 */
void testSaveIndexInformationForLaterUse(void)
{
  /* Preconditions */
  sdlogger_spi.status = SDLogger_RetreivingIndex;
  /* SD Card buffer contains relevant values */
  /* Next available address: */
  helperAssignUint(&sdcard1.input_buf[0], 0x12345678);
  /* Last completed log: */
  sdcard1.input_buf[4] = 12;

  /* Simulated callback from SD Card read function */
  sdlogger_spi_direct_index_received();

  /* Verify correct values set by function */
  TEST_ASSERT_EQUAL(0x00004000, sdlogger_spi.next_available_address);
  TEST_ASSERT_EQUAL(0, sdlogger_spi.last_completed);
  /* State change */
  TEST_ASSERT_EQUAL(SDLogger_Ready, sdlogger_spi.status);
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
  sdcard1.status = SDCard_Idle;
  /* Logger is ready as well: */
  sdlogger_spi.status = SDLogger_Ready;
  sdlogger_spi.next_available_address = 0x00004000;
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
  TEST_ASSERT_EQUAL(SDLogger_Logging, sdlogger_spi.status);
  /* LED is on */
#ifdef LOGGER_LED
  TEST_ASSERT_TRUE(LED_STATUS(LOGGER_LED));
#endif
}

void testOnlyStartMultiwriteOnceWhenSwitchIsFlipped(void) {
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Ready;
  sdcard1.status = SDCard_Idle;
  /* Index already available: */
  sdlogger_spi.next_available_address = 0x00004000;
  /* Switch is now state ON: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_start_Expect(&sdcard1, 0x00004000);

  /* First Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_Logging, sdlogger_spi.status);

  /* Expections second run */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* Do not expect multiwrite_start again */

  /* Second periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_Logging, sdlogger_spi.status);
}

/**
 * @brief testDoNotStartMultiWriteIfSdCardIsNotIdle
 * Only start logging if the SD Card is in idle state.
 */
void testDoNotStartMultiWriteIfSdCardIsNotIdle(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Ready;
  sdcard1.status = SDCard_Busy;
  sdlogger_spi.next_available_address = 0x00004000;
  /* Switch is now state ON: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  /* Don't call multiwrite_start because card is busy */

  /* First Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_Ready, sdlogger_spi.status);

  /* Card changes state to Idle */
  sdcard1.status = SDCard_Idle;

  /* Expectation: */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_start_Expect(&sdcard1, 0x00004000);

  /* Second periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_Logging, sdlogger_spi.status);
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
  sdlogger_spi.status = SDLogger_Ready;
  /* Switch in state OFF: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = -500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  /* State remains unchanged */
  TEST_ASSERT_EQUAL(SDLogger_Ready, sdlogger_spi.status);
}

/**
 * @brief testCheckFreeSpaceNotLogging
 * If not logging, return FALSE to prevent further calls to write.
 */
void testCheckFreeSpaceNotLogging(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Not accepting messages */
  sdlogger_spi.status = SDLogger_Ready;
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
  sdlogger_spi.status = SDLogger_Logging;
  sdlogger_spi.sdcard_buf_idx = 1;
  sdlogger_spi.idx = 0;

  /* Function call (from messages.h) */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       20);

  /* There is space available for writing */
  TEST_ASSERT_TRUE(available);
}

void testCheckFreeSpaceRequestJustTheAvailableBytes(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Logging;
  /* SDCard buffer full, logger buffer almost full */
  sdlogger_spi.sdcard_buf_idx = 513;
  sdlogger_spi.idx = SDLOGGER_BUFFER_SIZE - 4;

  /* Requesting free space */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       4);
  /* There is (just) enough space */
  TEST_ASSERT_TRUE(available);
}

void testCheckFreeSpaceRequestOneTooManyBytes(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Logging;
  /* SDCard buffer full, logger buffer almost full */
  sdlogger_spi.sdcard_buf_idx = 513;
  sdlogger_spi.idx = SDLOGGER_BUFFER_SIZE - 4;

  /* Requesting free space */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       5);
  /* There is (just) not enough space */
  TEST_ASSERT_FALSE(available);
}

void testCheckFreeSpaceBothBuffersPossible(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Logging;
  sdlogger_spi.sdcard_buf_idx = 510;
  sdlogger_spi.idx = 0;

  /* Requesting free space */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       67);
  /* There is (just) enough space */
  TEST_ASSERT_TRUE(available);
}

void testCheckFreeSpaceBothBuffersJustNotPossible(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Logging;
  sdlogger_spi.sdcard_buf_idx = 510;
  sdlogger_spi.idx = 0;

  /* Requesting free space */
  bool_t available = sdlogger_spi_direct_check_free_space(
                       sdlogger_spi.device.periph,
                       68);
  /* There is (just) not enough space */
  TEST_ASSERT_FALSE(available);
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
  sdlogger_spi.status = SDLogger_Logging;

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
  sdlogger_spi.status = SDLogger_Logging;
  /* Last element in the buffer: */
  sdlogger_spi.sdcard_buf_idx = 512;
  sdcard1.status = SDCard_MultiWriteIdle;

  /* Expectations */
  sdcard_spi_multiwrite_next_Expect(&sdcard1,
                                    &sdlogger_spi_direct_multiwrite_written);

  /* Put byte call through messages.h and pprzlog_tp */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xAB);

  /* Values set in sd buffer, index reset */
  TEST_ASSERT_EQUAL(0xAB, sdcard1.output_buf[512]);

  /* Write another byte after SD Card buffer is full */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xEF);
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0x4F);

  /* It should end up in the loggers internal buffer to copy later */
  TEST_ASSERT_EQUAL(0xEF, sdlogger_spi.buffer[0]);
  TEST_ASSERT_EQUAL(0x4F, sdlogger_spi.buffer[1]);
}

/**
 * @brief testSDBufferGetsFullButCardIsNotReady
 * If the SD Card buffer is full, it wants to write to the card. This can only
 * be done if the SD Card is ready to accept a data block.
 */
void testSDBufferGetsFullButCardIsNotReady(void)
{
  /* Preconditions */
  helperInitializeLogger();
  /* Last element in the buffer: */
  sdlogger_spi.sdcard_buf_idx = 512;
  /* SD Card is busy with previous block */
  sdcard1.status = SDCard_MultiWriteBusy;

  /* Expectations */
  /* SD Card busy, so don't call now */

  /* Put byte call through messages.h and pprzlog_tp */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xAB);
  /* The SD Card output buffer is now FULL */
}

/**
 * @brief testPeriodicIfSDCardIsReadyAndSDBufferIsFull
 * If the SD Card buffer is not written immediately when it got full (because
 * the card was still working on something else), check the sdcard status every
 * periodic loop and write it when the card is back to idle
 */
void testPeriodicIfSDCardIsReadyAndSDBufferIsFull(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_Logging;
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = 500;
  /* Buffer is full */
  sdlogger_spi.sdcard_buf_idx = 513;
  sdcard1.status = SDCard_MultiWriteBusy;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop #1 */
  sdlogger_spi_direct_periodic();

  /* SD Card status changed */
  sdcard1.status = SDCard_MultiWriteIdle;

  /* New expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_next_Expect(&sdcard1,
                                    &sdlogger_spi_direct_multiwrite_written);

  /* Periodic loop #2 */
  sdlogger_spi_direct_periodic();
}

/**
 * @brief testDoNotWriteIfLoggerBufferIsFull
 * If correctly implemented, the internal buffer will never get full because
 * proper implementation calls check_free_space. But if it is badly implemented,
 * it can be desastrous to write outside the buffer, so lets check it just in
 * case.
 */
void testDoNotWriteIfLoggerBufferIsFull(void)
{
  /* Pre-conditions */
  helperInitializeLogger();
  /* Last element in the buffer: */
  sdlogger_spi.sdcard_buf_idx = 513;
  sdlogger_spi.idx = SDLOGGER_BUFFER_SIZE - 1;

  /* Put byte call through messages.h and pprzlog_tp */
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xAB);
  sdlogger_spi_direct_put_byte(sdlogger_spi.device.periph, 0xCD);

  TEST_ASSERT_EQUAL(0xAB, sdlogger_spi.buffer[SDLOGGER_BUFFER_SIZE-1]);
  /* The index would get overwritten if not checking for end of buffer */
  TEST_ASSERT_EQUAL(SDLOGGER_BUFFER_SIZE, sdlogger_spi.idx);
}

/**
 * @brief testCopyLoggerBufferToSDCardWhenSpiTransactionFinished
 * The callback is called when the SPI transaction is complete. The SD Card will
 * still be in a busy state, but the output buffer can be used again for writing
 * data to.
 */
void testCopyLoggerBufferToSDCardWhenSpiTransactionFinished(void)
{
  /* Pre-conditions */
  helperInitializeLogger();
  /* Some stuff in the SD Logger buffer */
  sdlogger_spi.buffer[0] = 0xA2;
  sdlogger_spi.buffer[5] = 0xA7;
  sdlogger_spi.buffer[SDLOGGER_BUFFER_SIZE-1] = 0x42;
  sdlogger_spi.idx = SDLOGGER_BUFFER_SIZE;
  sdlogger_spi.sdcard_buf_idx = 513;
  sdlogger_spi.log_len = 5000;

  /* Callback when SPI transaction is complete */
  sdlogger_spi_direct_multiwrite_written();

  /* Check if stuff is copied from logger buffer to sdcard buffer */
  TEST_ASSERT_EQUAL_HEX(0xA2, sdcard1.output_buf[0+1]);
  TEST_ASSERT_EQUAL_HEX(0xA7, sdcard1.output_buf[5+1]);
  TEST_ASSERT_EQUAL_HEX(0x42, sdcard1.output_buf[SDLOGGER_BUFFER_SIZE-1+1]);

  /* Also make sure both indexes are now set to new correct values */
  TEST_ASSERT_EQUAL(SDLOGGER_BUFFER_SIZE+1, sdlogger_spi.sdcard_buf_idx);
  TEST_ASSERT_EQUAL(0, sdlogger_spi.idx);
  /* Check increment in log length, because another block was logged */
  TEST_ASSERT_EQUAL(5001, sdlogger_spi.log_len);
}

void testStopLogging(void)
{
  /* Preconditions */
  sdlogger_spi.status = SDLogger_Logging;
  /* New switch state is OFF: */
  radio_control.values[SDLOGGER_CONTROL_SWITCH] = -500;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Check in periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_LoggingFinalBlock, sdlogger_spi.status);
}

void testLoggingFinalBlockWithFullBuffersCardBusy(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_LoggingFinalBlock;
  sdcard1.status = SDCard_MultiWriteBusy;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  /* Status unchanged */
  TEST_ASSERT_EQUAL(SDLogger_LoggingFinalBlock, sdlogger_spi.status);
}

void testLoggingFinalBlockWithFullBuffersCardAvailable(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_LoggingFinalBlock;
  sdcard1.status = SDCard_MultiWriteIdle;
  /* Still a lot of data in the buffers */
  sdlogger_spi.idx = 20;
  sdlogger_spi.sdcard_buf_idx = 513;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_next_Expect(&sdcard1,
                                    &sdlogger_spi_direct_multiwrite_written);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();
}

/**
 * @brief testLoggingFinalBlockWithHalfFullBufferCardAvailalble
 * If there is some data in the buffer and card is idle, then fill the rest
 * with trailing zero's and submit.
 */
void testLoggingFinalBlockWithHalfFullBufferCardAvailalble(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_LoggingFinalBlock;
  sdcard1.status = SDCard_MultiWriteIdle;
  /* Some data in buffer */
  sdlogger_spi.idx = 0;
  sdlogger_spi.sdcard_buf_idx = 30;
  /* Until this value should not be overwritten with zero's */
  sdcard1.output_buf[29] = 0xEE;
  /* Set wrong values here, should be converted to trailing zero's */
  sdcard1.output_buf[30] = 0xB0;
  sdcard1.output_buf[31] = 0xAA;
  sdcard1.output_buf[512] = 0xDD;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_next_Expect(&sdcard1, &sdlogger_spi_direct_multiwrite_written);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL_HEX(0xEE, sdcard1.output_buf[29]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[30]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[31]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[512]);
}

/**
 * @brief testLoggingFinalBlockBuffersEmpty
 * When all buffers are empty in the LogginFinalBlock state, it is time to call
 * the multiwrite_stop function.
 */
void testLoggingFinalBlockBuffersEmpty(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.sdcard_buf_idx = 1;
  sdlogger_spi.idx = 0;
  sdlogger_spi.status = SDLogger_LoggingFinalBlock;
  sdcard1.status = SDCard_MultiWriteIdle;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_multiwrite_stop_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_StoppedLogging, sdlogger_spi.status);
}

void testWaitWhileReadyWithStoppingMultiWrite(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_StoppedLogging;
  sdcard1.status = SDCard_Busy;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_StoppedLogging, sdlogger_spi.status);
}

/**
 * @brief testReadyFinishingMultiWriteThenRequestIndexPage
 * As soon as the entire log is successfully written after a stop command, the
 * information on this new log needs to be put in the index. Therefore, the
 * index is downloaded first.
 */
void testReadyFinishingMultiWriteThenRequestIndexPage(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_StoppedLogging;
  sdcard1.status = SDCard_Idle;

  /* Expectations */
  sdcard_spi_periodic_Expect(&sdcard1);
  sdcard_spi_read_block_Expect(&sdcard1, 0x00002000, &sdlogger_spi_direct_index_received);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_GettingIndexForUpdate, sdlogger_spi.status);
}

/**
 * @brief callbackIndexReceivedWhileReadyForUpdatingIt
 * @param sdcard
 * @param addr
 * @param callback
 * @param cmock_num_calls
 * Callback to test values in sdcard_spi call in the unittest below.
 */
void callbackIndexReceivedWhileReadyForUpdatingIt(struct SDCard* sdcard, uint32_t addr, int cmock_num_calls)
{
  (void) sdcard; (void) addr; (void) cmock_num_calls;

  /* Check values in the output buffer */
  /* Next_available_address incremented by 1024: */
  TEST_ASSERT_EQUAL_HEX(0x12, sdcard1.output_buf[0+5]);
  TEST_ASSERT_EQUAL_HEX(0x34, sdcard1.output_buf[1+5]);
  TEST_ASSERT_EQUAL_HEX(0x5A, sdcard1.output_buf[2+5]);
  TEST_ASSERT_EQUAL_HEX(0x56, sdcard1.output_buf[3+5]);
  TEST_ASSERT_EQUAL(2, sdcard1.output_buf[4+5]);

  /* Start address and length written at dedicated location for log number 2 */
  TEST_ASSERT_EQUAL_HEX(0x12, sdcard1.output_buf[17+5]);
  TEST_ASSERT_EQUAL_HEX(0x34, sdcard1.output_buf[18+5]);
  TEST_ASSERT_EQUAL_HEX(0x56, sdcard1.output_buf[19+5]);
  TEST_ASSERT_EQUAL_HEX(0x56, sdcard1.output_buf[20+5]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[21+5]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[22+5]);
  TEST_ASSERT_EQUAL_HEX(0x04, sdcard1.output_buf[23+5]);
  TEST_ASSERT_EQUAL_HEX(0x00, sdcard1.output_buf[24+5]);
}

/**
 * @brief testIndexReceivedWhileReadyForUpdatingIt
 * Logging was finished, now the index is received. The input buffer is copied
 * to the output buffer and appropriate values are updated.
 */
void testIndexReceivedWhileReadyForUpdatingIt(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_GettingIndexForUpdate;
  sdcard1.status = SDCard_Idle;
  /* Previous data in the index: */
  sdlogger_spi.next_available_address = 0x12345656;
  sdlogger_spi.last_completed = 1;
  sdcard1.input_buf[4] = 1;
  sdlogger_spi.log_len = 1024;
  for (uint16_t i = 5; i < SD_BLOCK_SIZE; i++) {
    sdcard1.input_buf[i] = 0x00;
  }

  /* Expectations */
  sdcard_spi_write_block_Expect(&sdcard1, 0x00002000);
  sdcard_spi_write_block_StubWithCallback(&callbackIndexReceivedWhileReadyForUpdatingIt);

  /* Index received callback */
  sdlogger_spi_direct_index_received();

  /* Next address updated for next log */
  TEST_ASSERT_EQUAL(0x12345A56, sdlogger_spi.next_available_address);
  /* Log length reset */
  TEST_ASSERT_EQUAL(0, sdlogger_spi.log_len);
  TEST_ASSERT_EQUAL(SDLogger_UpdatingIndex, sdlogger_spi.status);
}

void testKeepUpdatingIndex(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_UpdatingIndex;
  sdcard1.status = SDCard_Busy;

  /* Expecations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();
}

void testReadyUpdatingIndex(void)
{
  /* Preconditions */
  helperInitializeLogger();
  sdlogger_spi.status = SDLogger_UpdatingIndex;
  sdcard1.status = SDCard_Idle;
  /* Led is on, should go off */
#ifdef LOGGER_LED
  LED_SET(LOGGER_LED, TRUE);
#endif
  /* Expecations */
  sdcard_spi_periodic_Expect(&sdcard1);

  /* Periodic loop */
  sdlogger_spi_direct_periodic();

  TEST_ASSERT_EQUAL(SDLogger_Ready, sdlogger_spi.status);
#ifdef LOGGER_LED
  TEST_ASSERT_FALSE(LED_STATUS(LOGGER_LED));
#endif
}
