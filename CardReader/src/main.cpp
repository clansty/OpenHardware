#include <Arduino.h>
#include "Aime_Reader.h"

constexpr u8 UID_LENGTH = 8;
u8 prevIDm[UID_LENGTH];
u32 prevTime;

void setup()
{
    // Add initial delay to allow the serial monitor to catch up
    delay(500);
    // led_animation();

    // Initialize serial port
    USBSerial.begin(115200);

    // Initialize I2C communication
    Wire.setPins(GPIO_NUM_4, GPIO_NUM_5);

    // Initialize the LED
    CFastLED::addLeds<NEOPIXEL, GPIO_NUM_6>(leds, NUM_LEDS);
    FastLED.setBrightness(BR_DIM);

    // Find the PN532 NFC module
    nfc.begin();
    u32 versiondata;
    while ((versiondata = nfc.getFirmwareVersion()) == 0)
    {
        USBSerial.println("Didn't find PN53x board");
        delay(100);
    }

    // Set the max number of retry attempts to read from a card
    // This prevents us from waiting forever for a card, which is the default behaviour of the PN532.
    nfc.setPassiveActivationRetries(0x10);
    nfc.SAMConfig();

    // Clear the IDm buffer
    memset(req.bytes, 0, sizeof(req.bytes));
    memset(res.bytes, 0, sizeof(res.bytes));
}

void led_animation()
{
    FastLED.setBrightness(BR_BRIGHT);
    CRGB colors[] = {CRGB::LimeGreen, CRGB::Black, CRGB::Gold, CRGB::Black};

    for (const auto color : colors)
    {
        for (u8 i = 1; i < NUM_LEDS; i++)
        {
            leds[i] = color;
            FastLED.show();
            delay(35);
        }
    }

    FastLED.setBrightness(BR_DIM);
}

void led_animation(CRGB color)
{
    FastLED.setBrightness(BR_BRIGHT);
    for (u8 i = 1; i < NUM_LEDS; i++)
    {
        leds[i] = color;
        FastLED.show();
        delay(100);
    }

    FastLED.setBrightness(BR_DIM);
}

void foundCard(const u8 *uid, const u8 len, const char *cardType)
{
    // Check if the same card is present
    if (memcmp(uid, prevIDm, UID_LENGTH) == 0 && millis() - prevTime < 3000)
    {
        delay(5);
        return;
    }

    USBSerial.printf("\nFound a %s card!\n", cardType);
    USBSerial.print("UID Value: ");
    for (u8 i = 0; i < len; i++)
    {
        if (uid[i] < 0x10)
            USBSerial.print("0");
        USBSerial.print(uid[i], HEX);
    }
    USBSerial.println("");

    led_animation();

    memcpy(prevIDm, uid, UID_LENGTH);
    prevTime = millis();
}

void loop()
{
    switch (packet_read())
    {
    case 0:
        break;

    case CMD_TO_NORMAL_MODE:
        sys_to_normal_mode();
        break;
    case CMD_GET_FW_VERSION:
        sys_get_fw_version();
        break;
    case CMD_GET_HW_VERSION:
        sys_get_hw_version();
        break;

    // Card read
    case CMD_START_POLLING:
        nfc_start_polling();
        led_animation();
        break;
    case CMD_STOP_POLLING:
        nfc_stop_polling();
        break;
    case CMD_CARD_DETECT:
        nfc_card_detect();
        break;

    // MIFARE
    case CMD_MIFARE_KEY_SET_A:
        memcpy(KeyA, req.key, 6);
        res_init();
        break;

    case CMD_MIFARE_KEY_SET_B:
        res_init();
        memcpy(KeyB, req.key, 6);
        break;

    case CMD_MIFARE_AUTHORIZE_A:
        nfc_mifare_authorize_a();
        break;

    case CMD_MIFARE_AUTHORIZE_B:
        nfc_mifare_authorize_b();
        break;

    case CMD_MIFARE_READ:
        nfc_mifare_read();
        break;

    // FeliCa
    case CMD_FELICA_THROUGH:
        nfc_felica_through();
        break;

    // LED
    case CMD_EXT_BOARD_LED_RGB:
        FastLED.showColor(CRGB(req.color_payload[0], req.color_payload[1], req.color_payload[2]));
        break;

    case CMD_EXT_BOARD_INFO:
        sys_get_led_info();
        break;

    case CMD_EXT_BOARD_LED_RGB_UNKNOWN:
        break;

    default:
        res_init();
    }
    packet_write();
}
