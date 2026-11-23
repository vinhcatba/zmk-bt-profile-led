/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 */

#pragma once

/**
 * @brief Trigger the all-LED quick blink animation for clearing all BT profiles.
 *
 * After the quick-blink sequence finishes, the widget automatically resumes
 * indicating the active Bluetooth profile's current connection status.
 */
void bt_profile_led_indicate_clear_all(void);

/**
 * @brief Force an immediate refresh of the active Bluetooth profile LED state.
 */
void bt_profile_led_refresh(void);
