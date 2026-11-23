#include "standby.h"

#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include <utility/M5IOE1_Class.hpp>

#include "ui.h"

using m5::M5IOE1_Class;
using m5::pwm_polarity_t;

namespace standby {
namespace {
constexpr gpio_num_t kKeyA = GPIO_NUM_2;
constexpr gpio_num_t kKeyB = GPIO_NUM_1;
constexpr gpio_num_t kPm1Irq = GPIO_NUM_12;
constexpr gpio_num_t kTouchIrq = GPIO_NUM_13;
constexpr uint8_t kImuAddr = 0x68;    // BMI270
constexpr uint8_t kPm1Addr = 0x6E;    // M5PM1
constexpr uint8_t kIoe1Addr = 0x4F;   // M5IOE1
constexpr uint8_t kCst820Addr = 0x15;
constexpr uint32_t kI2cFreq = 100000;

bool g_wokeByButton = false;

// レジスタの下位 4 ビットだけ書き換える(PM1 0x09 / IOE1 0x23 の I2C スリープ秒)
bool setLowNibble(uint8_t addr, uint8_t reg, uint8_t value) {
  uint8_t cur = 0;
  if (!M5.In_I2C.readRegister(addr, reg, &cur, 1, kI2cFreq)) return false;
  const uint8_t next = static_cast<uint8_t>((cur & 0xF0) | (value & 0x0F));
  return M5.In_I2C.writeRegister8(addr, reg, next, kI2cFreq);
}
}  // namespace

void prepareAfterBoot() {
  const auto cause = esp_sleep_get_wakeup_cause();
  g_wokeByButton = (cause == ESP_SLEEP_WAKEUP_EXT1);
  if (cause != ESP_SLEEP_WAKEUP_UNDEFINED) {
    // ディープスリープ明けは RTC IO が握ったままで、通常の GPIO 入力が読めない。元に戻す
    rtc_gpio_deinit(kKeyA);
    rtc_gpio_deinit(kKeyB);
    rtc_gpio_deinit(kPm1Irq);
    rtc_gpio_deinit(kTouchIrq);
    log_i("[standby] ディープスリープから復帰 cause=%d ext1=0x%llx", static_cast<int>(cause),
          static_cast<unsigned long long>(esp_sleep_get_ext1_wakeup_status()));
  }
}

bool wokeByButton() { return g_wokeByButton; }

const char* wakeReason() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1: return "deep sleep → KEYA/KEYB";
    case ESP_SLEEP_WAKEUP_UNDEFINED: return "power on / reset";
    default: return "deep sleep → other";
  }
}

void enter() {
  log_i("[standby] スタンバイに入る(KEYA / KEYB で復帰)");
  M5.Display.sleep();
  M5.Display.waitDisplay();

  // タッチコントローラをスリープ(CST820 のみ。CST9217 は未確認なので触らない)
  if (auto* t = M5.Display.touch()) {
    if (t->config().i2c_addr == kCst820Addr) M5.In_I2C.writeRegister8(kCst820Addr, 0xE5, 0x03, kI2cFreq);
  }

  // BMI270 をサスペンド(公式手順: PWR_CONF=0x01 → 100ms → PWR_CTRL=0x00)
  M5.In_I2C.writeRegister8(kImuAddr, 0x7C, 0x01, kI2cFreq);
  delay(100);
  M5.In_I2C.writeRegister8(kImuAddr, 0x7D, 0x00, kI2cFreq);

  // IOE1: 表示電源(4)、コーデック(3)、アンプ(10)、振動(9)を初期状態のハイインピーダンスに戻す
  {
    auto& ioe = static_cast<M5IOE1_Class&>(M5.getIOExpander(0));
    ioe.setPwmDuty12bit(M5IOE1_Class::pwm_ch1, 0, pwm_polarity_t::normal, false);
    for (uint8_t pin : {M5IOE1_Class::gpio4, M5IOE1_Class::gpio3, M5IOE1_Class::gpio10, M5IOE1_Class::gpio9}) {
      ioe.setHighImpedance(pin, true);
    }
  }

  // PM1: Grove 5V ブーストと LED を切る(充電は止めない)
  M5.Power.M5pm1.setExtOutput(false);
  M5.Power.M5pm1.setLedEnLevel(false);

  // 起床は KEYA(G2)/ KEYB(G1)の Low。使わない割り込み線は切り離す
  esp_sleep_enable_ext1_wakeup((1ULL << kKeyA) | (1ULL << kKeyB), ESP_EXT1_WAKEUP_ANY_LOW);
  rtc_gpio_isolate(kPm1Irq);
  rtc_gpio_isolate(kTouchIrq);

  // 最後に I2C アイドルスリープ(1 秒)を有効にし、1 秒 I2C を使わずに待ってから眠る
  setLowNibble(kIoe1Addr, 0x23, 1);
  setLowNibble(kPm1Addr, 0x09, 1);
  delay(1100);
  esp_deep_sleep_start();
}

}  // namespace standby
