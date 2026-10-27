#include "Gpio315Ook.h"

#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "soc/soc.h"

#if CONFIG_IDF_TARGET_ESP32
#include "soc/io_mux_reg.h"
#include "soc/i2s_reg.h"
#include "soc/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
#include "esp_rom_gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
// Some Arduino core versions use DR_REG_I2S_BASE in the register macros.
#ifndef DR_REG_I2S_BASE
#define DR_REG_I2S_BASE DR_REG_I2S0_BASE
#endif
#endif

uint8_t Gpio315Ook::pin() const {
#if CONFIG_IDF_TARGET_ESP32
  return 0;
#elif CONFIG_IDF_TARGET_ESP32C3
  return 9;
#elif CONFIG_IDF_TARGET_ESP32S3
  return 4;
#else
  return 255;
#endif
}

bool Gpio315Ook::ready() const { return ready_; }

void Gpio315Ook::carrier(bool on) {
#if CONFIG_IDF_TARGET_ESP32
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,
                  on ? FUNC_GPIO0_CLK_OUT1 : PIN_FUNC_GPIO);
#elif CONFIG_IDF_TARGET_ESP32C3
  // Changing the matrix selector directly keeps OOK pulse edges short.
  REG_SET_FIELD(GPIO_FUNC9_OUT_SEL_CFG_REG, GPIO_FUNC9_OUT_SEL,
                on ? I2S_MCLK_OUT_IDX : SIG_GPIO_OUT_IDX);
#elif CONFIG_IDF_TARGET_ESP32S3
  REG_SET_FIELD(GPIO_FUNC4_OUT_SEL_CFG_REG, GPIO_FUNC4_OUT_SEL,
                on ? I2S0_MCLK_OUT_IDX : SIG_GPIO_OUT_IDX);
#else
  (void)on;
#endif
}

void Gpio315Ook::pulse(uint32_t highUs, uint32_t lowUs) {
  carrier(true);
  delayMicroseconds(highUs);
  carrier(false);
  delayMicroseconds(lowUs);
}

bool Gpio315Ook::setClockEnabled(bool enabled) {
#if CONFIG_IDF_TARGET_ESP32C3
  if (enabled) {
    REG_SET_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_CLK_EN);
    REG_SET_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLK_ACTIVE);
    REG_SET_BIT(I2S_TX_CONF_REG(0), I2S_TX_UPDATE);
    for (int i = 0; i < 10000; ++i) {
      if (!(REG_READ(I2S_TX_CONF_REG(0)) & I2S_TX_UPDATE)) return true;
      delayMicroseconds(1);
    }
    return false;
  }
  // Leave GPIO on its low software output before removing the clock.
  carrier(false);
  REG_CLR_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLK_ACTIVE);
  REG_CLR_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_CLK_EN);
#else
  (void)enabled;
#endif
  return true;
}

bool Gpio315Ook::begin() {
#if CONFIG_IDF_TARGET_ESP32
  constexpr gpio_num_t output = GPIO_NUM_0;
  gpio_set_direction(output, GPIO_MODE_OUTPUT);
  gpio_set_level(output, 0);
  periph_module_enable(PERIPH_I2S0_MODULE);
  // APLL ~126 MHz / 2 = ~63 MHz; fifth harmonic ~315 MHz.
  rtc_clk_apll_enable(true, 154, 153, 8, 0);
  REG_SET_BIT(I2S_CLKM_CONF_REG(0), I2S_CLKA_ENA);
  REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM, 2);
  REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A, 0);
  REG_SET_FIELD(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B, 0);
  REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);
  carrier(false);
#elif CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
#if CONFIG_IDF_TARGET_ESP32C3
  constexpr gpio_num_t output = GPIO_NUM_9;
  periph_module_enable(PERIPH_I2S1_MODULE);
#else
  constexpr gpio_num_t output = GPIO_NUM_4;
  periph_module_enable(PERIPH_I2S0_MODULE);
#endif
  gpio_reset_pin(output);
#if CONFIG_IDF_TARGET_ESP32C3
  // Matches the GPIO9 configuration verified with the lamp.
  gpio_set_direction(output, GPIO_MODE_INPUT_OUTPUT);
#else
  gpio_set_direction(output, GPIO_MODE_OUTPUT);
#endif
  gpio_set_drive_capability(output, GPIO_DRIVE_CAP_3);
  gpio_set_level(output, 0);
  gpio_matrix_out(output, SIG_GPIO_OUT_IDX, false, false);
#if CONFIG_IDF_TARGET_ESP32C3
  // 240 MHz / (2 + 2/7) = 105 MHz; third harmonic = 315 MHz.
  REG_SET_FIELD(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLK_SEL, 1);
  REG_SET_FIELD(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLKM_DIV_NUM, 2);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_X, 2);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_Y, 1);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_Z, 2);
  REG_CLR_BIT(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_YN1);
#else
  // S3 proposal: 160 MHz / (2 + 34/63) = 63 MHz; fifth harmonic = 315 MHz.
  REG_SET_FIELD(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLK_SEL, 2);
  REG_SET_FIELD(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLKM_DIV_NUM, 2);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_X, 1);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_Y, 5);
  REG_SET_FIELD(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_Z, 29);
  REG_SET_BIT(I2S_TX_CLKM_DIV_CONF_REG(0), I2S_TX_CLKM_DIV_YN1);
#endif
  // Shared MCLK selects the TX clock through the RX clock register.
  REG_CLR_BIT(I2S_RX_CLKM_CONF_REG(0), I2S_MCLK_SEL);
  REG_SET_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_CLK_EN);
  REG_SET_BIT(I2S_TX_CLKM_CONF_REG(0), I2S_TX_CLK_ACTIVE);
  REG_SET_BIT(I2S_TX_CONF_REG(0), I2S_TX_UPDATE);
  bool updated = false;
  for (int i = 0; i < 10000; ++i) {
    if (!(REG_READ(I2S_TX_CONF_REG(0)) & I2S_TX_UPDATE)) {
      updated = true;
      break;
    }
    delayMicroseconds(1);
  }
  carrier(false);
  if (!updated) return false;
#if CONFIG_IDF_TARGET_ESP32C3
  setClockEnabled(false);
#endif
#else
  return false;
#endif
  ready_ = true;
  return true;
}

bool Gpio315Ook::send(uint32_t code, uint8_t bits, uint16_t pulseUs,
                      uint8_t repeats) {
  if (!ready_ || bits < 1 || bits > 32 || pulseUs < 100 ||
      pulseUs > 2000 || repeats < 1 || repeats > 32) return false;
#if CONFIG_IDF_TARGET_ESP32C3
  if (!setClockEnabled(true)) {
    setClockEnabled(false);
    return false;
  }
#endif
  for (uint8_t repeat = 0; repeat < repeats; ++repeat) {
    for (int bit = bits - 1; bit >= 0; --bit) {
      const bool one = (code >> bit) & 1U;
      pulse((one ? 3 : 1) * pulseUs, (one ? 1 : 3) * pulseUs);
    }
    pulse(pulseUs, 31UL * pulseUs);
  }
  carrier(false);
#if CONFIG_IDF_TARGET_ESP32C3
  setClockEnabled(false);
#endif
  return true;
}
