# zmk-bt-profile-led

A [ZMK](https://zmk.dev/docs/development/module-creation) module based on [caksoylar/zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget) and [awkannan/zmk-smart-rgbled-widget](https://github.com/awkannan/zmk-smart-rgbled-widget) that indicates the active Bluetooth profile's connection state using **single-color GPIO LEDs** (`gpio-leds`) or an **addressable SK6812 / WS2812B LED strip** (`led_strip`) — with **one LED assigned per profile** and **no ZMK core fork required**.

## Behavior

| State | LED Behavior | Default Timing |
| --- | --- | --- |
| **Advertising / Empty profile** (open, waiting to pair) | Slow blink on the active profile's LED | 500 ms interval (`SLOW_BLINK_MS`) |
| **Disconnected** (bonded, not connected) | Fast blink on the active profile's LED | 150 ms interval (`FAST_BLINK_MS`) |
| **Connected** | Solid ON on the active profile's LED, then OFF to save battery | 5000 ms (5 s) solid ON |
| **Profile switched** (`BT_SEL`, `BT_NXT`, `BT_PRV`) | Immediately turns off the previous profile's LED and starts indicating the newly selected profile's LED | Immediate |
| **Clear all BT profiles** (`&bt_clr_all_led` or `&bt BT_CLR_ALL`) | Quick blink across **all** configured profile LEDs, then resumes active profile status | 5 blinks at 150 ms (`FAST_BLINK_MS`) |

## Installation

### 1. Add the module to `config/west.yml`

Push this folder to a GitHub repository (or reference it locally) and add it to `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: vinhcatba
      url-base: https://github.com/vinhcatba
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-bt-profile-led
      remote: vinhcatba
      revision: main
  self:
    path: config
```

### 2. Choose GPIO LEDs or SK6812 / WS2812B Strip

- **GPIO LEDs (default)**: set `CONFIG_BT_PROFILE_LED=y` (`CONFIG_BT_PROFILE_LED_STRIP` defaults to `n`).
- **SK6812 / WS2812B strip**: set `CONFIG_BT_PROFILE_LED=y` and `CONFIG_BT_PROFILE_LED_STRIP=y`.

#### GPIO LEDs vs. Addressable LED Strip

| Feature | Single-color GPIO LEDs (`gpio-leds`) | Addressable LED Strip (`led_strip`) |
| --- | --- | --- |
| **Config** | `CONFIG_BT_PROFILE_LED=y` (default backend) | `CONFIG_BT_PROFILE_LED=y` + `CONFIG_BT_PROFILE_LED_STRIP=y` |
| **Color & States** | Single color; states distinguished by blink pattern only | Per-state RGB colors (Connected, Advertising, Disconnected, Clear All) |
| **Brightness / Dimming** | Binary ON/OFF only (brightness set by hardware resistor) | Software brightness (`CONFIG_BT_PROFILE_LED_STRIP_BRIGHTNESS`, `1..255`) |
| **Pins & Wiring** | 1 GPIO pin per profile (`led-bt-0` .. `led-bt-7`) | 1 data pin (e.g., SPI MOSI) for all profile LEDs |
| **LED Ordering** | Mapped directly by `led-bt-N` devicetree aliases | Contiguous slice via `OFFSET`, `COUNT`, and `REVERSE` |
| **Idle Power Draw** | ~0 mA when OFF | ~0.5–1 mA quiescent current per LED even when dark (unless power-gated) |
| **ZMK Lighting Coexistence** | Independent of ZMK Backlight and RGB Underglow | Independent when on its own strip (`zmk,bt-profile-led-strip`); do **not** share one strip with `CONFIG_ZMK_RGB_UNDERGLOW=y` |

> **ZMK Lighting Isolation:** `zmk-bt-profile-led` brightness and color settings are completely local to this module and do **not** affect ZMK RGB Underglow (`CONFIG_ZMK_RGB_UNDERGLOW`), ZMK Backlight (`CONFIG_ZMK_BACKLIGHT`), or HID lock indicators. If you use both RGB Underglow and `zmk-bt-profile-led`, wire them as two separate `led_strip` devices (`zmk,underglow` and `zmk,bt-profile-led-strip`) so they do not overwrite each other's pixels.

#### Option A: Single-color GPIO LEDs (Default)

Define your LEDs under a `gpio-leds` node in your board/shield `.overlay` or `.dts` and map aliases `led-bt-0` through `led-bt-4` (up to `led-bt-7` supported):

```dts
/ {
    aliases {
        led-bt-0 = &bt_led0;
        led-bt-1 = &bt_led1;
        led-bt-2 = &bt_led2;
        led-bt-3 = &bt_led3;
        led-bt-4 = &bt_led4;
    };

    leds {
        compatible = "gpio-leds";
        status = "okay";

        bt_led0: bt_led_0 { gpios = <&gpio0 26 GPIO_ACTIVE_HIGH>; };
        bt_led1: bt_led_1 { gpios = <&gpio0 30 GPIO_ACTIVE_HIGH>; };
        bt_led2: bt_led_2 { gpios = <&gpio0 6  GPIO_ACTIVE_HIGH>; };
        bt_led3: bt_led_3 { gpios = <&gpio0 8  GPIO_ACTIVE_HIGH>; };
        bt_led4: bt_led_4 { gpios = <&gpio0 17 GPIO_ACTIVE_HIGH>; };
    };
};
```

> Use `GPIO_ACTIVE_LOW` if your LEDs are wired between VCC and the GPIO pin, or `GPIO_ACTIVE_HIGH` if wired between the GPIO pin and GND.

Enable in `config/<keyboard>.conf`:

```ini
CONFIG_BT_PROFILE_LED=y
```

#### Option B: Addressable SK6812 / WS2812B LED strip (`CONFIG_BT_PROFILE_LED_STRIP=y`)

In Zephyr / [ZMK Lighting](https://zmk.dev/docs/features/lighting), SK6812 LEDs use the `worldsemi,ws2812-spi` (or `ws2812-gpio` / `ws2812-i2s` / `ws2812-rpi_pico-pio`) driver. Configure your strip in your board `.overlay` (example for nRF52840 SPI3, matching [ZMK RGB Underglow hardware docs](https://zmk.dev/docs/hardware-integration/lighting/underglow)) and point `zmk,bt-profile-led-strip` (or `zmk,underglow`) to it:

```dts
#include <dt-bindings/led/led.h>

&pinctrl {
    spi3_default: spi3_default {
        group1 {
            psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
        };
    };

    spi3_sleep: spi3_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
            low-power-enable;
        };
    };
};

&spi3 {
    compatible = "nordic,nrf-spim";
    status = "okay";
    pinctrl-0 = <&spi3_default>;
    pinctrl-1 = <&spi3_sleep>;
    pinctrl-names = "default", "sleep";

    led_strip: ws2812@0 {
        compatible = "worldsemi,ws2812-spi";
        reg = <0>;
        spi-max-frequency = <4000000>;
        chain-length = <5>; /* number of SK6812 LEDs */
        spi-one-frame = <0x70>;
        spi-zero-frame = <0x40>;
        color-mapping = <LED_COLOR_ID_GREEN LED_COLOR_ID_RED LED_COLOR_ID_BLUE>;
    };
};

/ {
    chosen {
        zmk,bt-profile-led-strip = &led_strip;
    };
};
```

Enable in `config/<keyboard>.conf` (keep `CONFIG_ZMK_RGB_UNDERGLOW=n` if this strip is dedicated to status LEDs so underglow does not overwrite pixels):

```ini
CONFIG_BT_PROFILE_LED=y
CONFIG_BT_PROFILE_LED_STRIP=y
CONFIG_LED_STRIP=y
CONFIG_SPI=y
```

> **Note:** By default, profile `0` maps to pixel index `CONFIG_BT_PROFILE_LED_STRIP_OFFSET + 0`, profile `1` to `OFFSET + 1`, and so on up to `CONFIG_BT_PROFILE_LED_STRIP_COUNT`. Set `CONFIG_BT_PROFILE_LED_STRIP_REVERSE=y` to reverse this order (profile `0` maps to `OFFSET + COUNT - 1`).

### 3. Clear-all-profiles quick blink in your keymap

Because ZMK's built-in `zmk_ble_clear_all_bonds()` (called by `&bt BT_CLR_ALL` in [behavior_bt.c](https://github.com/zmkfirmware/zmk/blob/main/app/src/behaviors/behavior_bt.c)) does not emit a dedicated "clear all" event, this module provides two mechanisms:

1. **Guaranteed (recommended): `&bt_clr_all_led` drop-in behavior** — always quick-blinks all LEDs and clears all bonds, even when 0 or 1 profiles are currently paired:
   ```dts
   #include <behaviors/bt_profile_led.dtsi>

   / {
       keymap {
           compatible = "zmk,keymap";
           default_layer {
               bindings = <
                   ... &bt_clr_all_led ...
               >;
           };
       };
   };
   ```
2. **Automatic fallback for standard `&bt BT_CLR_ALL`** — enabled by default via `CONFIG_BT_PROFILE_LED_DETECT_MULTI_UNPAIR=y`, which detects when 2 or more bonds are deleted within 300 ms via Zephyr's `bond_deleted` callback and triggers the all-LED quick blink automatically.

## Kconfig Options

### General

| Option | Description | Default |
| --- | --- | --- |
| `CONFIG_BT_PROFILE_LED` | Enable the per-profile Bluetooth status LED widget (GPIO LEDs by default) | `n` |
| `CONFIG_BT_PROFILE_LED_STRIP` | Use addressable SK6812 / WS2812B LED strip (`led_strip`) instead of GPIO LEDs | `n` |
| `CONFIG_BT_PROFILE_LED_SLOW_BLINK_MS` | Slow blink interval in ms (ON and OFF duration for advertising / empty profile) | `500` |
| `CONFIG_BT_PROFILE_LED_FAST_BLINK_MS` | Fast blink interval in ms (ON and OFF duration for disconnected profile and clear-all quick blink) | `150` |
| `CONFIG_BT_PROFILE_LED_CONNECTED_DURATION_MS` | Solid ON duration in ms after connecting before turning off | `5000` |
| `CONFIG_BT_PROFILE_LED_CLEAR_ALL_BLINK_COUNT` | Number of quick blinks across all LEDs on clear-all | `5` |
| `CONFIG_BT_PROFILE_LED_DETECT_MULTI_UNPAIR` | Auto-detect standard `&bt BT_CLR_ALL` when >= 2 bonds are cleared | `y` |
| `CONFIG_BT_PROFILE_LED_SLEEP_TURN_OFF` | Turn off all profile LEDs when keyboard enters sleep | `y` |

### SK6812 / WS2812B Strip Options (when `CONFIG_BT_PROFILE_LED_STRIP=y`)

| Option | Description | Default |
| --- | --- | --- |
| `CONFIG_BT_PROFILE_LED_STRIP_OFFSET` | Starting pixel index on the strip for BT profile 0 | `0` |
| `CONFIG_BT_PROFILE_LED_STRIP_COUNT` | Number of strip pixels assigned to BT profiles (`1..8`) | `5` |
| `CONFIG_BT_PROFILE_LED_STRIP_REVERSE` | Reverse strip LED order (profile `0` maps to last profile LED) | `n` |
| `CONFIG_BT_PROFILE_LED_STRIP_BRIGHTNESS` | Brightness level (`1..255`) | `32` |
| `CONFIG_BT_PROFILE_LED_STRIP_COLOR_CONNECTED` | Color for connected state | Blue (`4`) |
| `CONFIG_BT_PROFILE_LED_STRIP_COLOR_ADVERTISING` | Color for advertising / empty profile | Yellow (`3`) |
| `CONFIG_BT_PROFILE_LED_STRIP_COLOR_DISCONNECTED` | Color for disconnected bonded profile | Red (`1`) |
| `CONFIG_BT_PROFILE_LED_STRIP_COLOR_CLEAR_ALL` | Color for clear-all quick blink | Red (`1`) |

Color values: `0` = Black, `1` = Red, `2` = Green, `3` = Yellow, `4` = Blue, `5` = Magenta, `6` = Cyan, `7` = White.
