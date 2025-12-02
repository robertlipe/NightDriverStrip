# WiFi State Management and Captive Portal Design

## Overview

This document outlines the design, implementation details, and rationale behind the WiFi state machine and Access Point (AP) captive portal functionality. Its purpose is to explain the desired user experience for WiFi configuration and to serve as a technical reference for maintaining and extending this system.

## 1. Core Scenarios & Design Goals

The primary goal of the WiFi subsystem is to reliably connect to a known network while providing a user-friendly fallback for configuration when needed. The logic is designed around several key real-world scenarios.

### 1.1. The "Harrie Case" (First-Time Setup / No Known Network)

*   **Scenario:** The device has no stored WiFi credentials, or no known SSIDs are visible.
*   **Goal:** Quickly provide the user with a way to configure the device.
*   **Behavior:** The system attempts to connect for a **short timeout** (e.g., 30 seconds). Upon failure (specifically `WL_NO_SSID_AVAIL`), it immediately launches the captive portal AP.

### 1.2. The "Dave Case" (Known Network Temporarily Unavailable)

*   **Scenario:** The device has valid credentials for a network that is temporarily unreachable (e.g., router rebooting, temporary outage).
*   **Goal:** Patiently wait for the known network to return without forcing the user to reconfigure.
*   **Behavior:** The system will continuously attempt to reconnect for a **long timeout** (e.g., 15 minutes). Only after this extended period will it give up and launch the captive portal AP.

### 1.3. The "Mistyped Password Case" (Authentication Failure)

*   **Scenario:** The device is trying to connect to a visible, known SSID, but authentication fails due to incorrect credentials.
*   **Goal:** Fail fast and allow the user to quickly re-enter their credentials.
*   **Behavior:** The system attempts to connect for a **short timeout** (e.g., 30 seconds). Upon failure (specifically `WL_CONNECT_FAILED`), it launches the captive portal AP.

### 1.4. Robust Mode Transitions

*   **Scenario:** The system needs to switch between WiFi modes (e.g., from Station `WIFI_STA` to Access Point `WIFI_AP`). Forceful or rapid transitions, such as issuing a `startportal` command after `clearsettings`, could historically cause the WiFi stack to "wedge" or become unresponsive.
*   **Goal:** Ensure all mode transitions are stable and reliable.
*   **Behavior:** All functions that change the WiFi mode now use a hardened helper function (`SetWiFiMode`) that explicitly handles tearing down the previous mode before setting the new one, polling to confirm the change.

## 2. Implemented Solution

To achieve the behavior described above, the following key components were implemented.

### 2.1. State-Aware Timeouts

The main network loop in `src/network.cpp` (`NetworkHandlingLoopEntry`) inspects the connection result from the WiFi driver (`WiFi.status()`). Based on the status code (`WL_NO_SSID_AVAIL`, `WL_CONNECT_FAILED`, `WL_DISCONNECTED`, etc.), it dynamically selects either the short or long timeout before deciding to launch the captive portal. This logic is the core of how the "Harrie," "Dave," and "Mistyped Password" cases are handled.

### 2.2. Robust WiFi Mode Switching

The `SetWiFiMode` function in `src/network.cpp` was enhanced to be more robust. It now ensures a clean teardown of the current WiFi mode before attempting to set a new one, polls to verify the mode change was successful, and includes delays to allow the WiFi stack to stabilize. This prevents the "wedging" issue seen in rapid state transitions.

### 2.3. Automated On-Device Testing

A comprehensive, on-device testing framework was created (`src/wifi_test.cpp`, `include/wifi_test.h`). These tests automate the process of putting the device into each of the key scenarios (Harrie, Mistyped Password, etc.) and verify that the system behaves as expected. This is crucial for preventing future regressions in the WiFi state machine.

## 3. Future Work

### 3.1. AP Inactivity Timeout

**Assessment:** This is a **medium-sized** task with **medium risk**.

*   **Size (Medium):** The change will require coordinated modifications across roughly 5 files (`network.cpp`, `webserver.h`, `webserver.cpp`, `deviceconfig.h`, `deviceconfig.cpp`) to introduce the new timer, the configuration setting, and the timeout logic itself.
*   **Risk (Medium):** The primary risk comes from modifying the main network state loop in `src/network.cpp`. This is a critical part of the system, and any changes have the potential to introduce unintended behavior, such as a reboot loop if the device times out but can't find a network to connect to. The risk is manageable, but requires careful implementation.

**Proposed Implementation Plan:**

1.  **Add an Activity Tracker:** A new timestamp variable (`_lastActivityMillis`) needs to be added to the `CWebServer` class to track the last time a user interacted with the captive portal.
2.  **Record User Activity:** The web server's captive portal request handlers need to be updated to call a new `RecordActivity()` method, which will update this timestamp on every user request.
3.  **Add a Configurable Timeout:** A new setting will be added to the `DeviceConfig` system to allow the idle timeout period to be configured and saved.
4.  **Implement the Timeout Logic:** In the main `NetworkHandlingLoopEntry` function, when the device is in captive portal mode, a check will be added. If the time since the last user activity exceeds the configured timeout, it will trigger the timeout action.
5.  **Trigger a Reboot:** The safest and most robust way to exit the portal and restart the connection process is to use the web server's existing `RequestReboot()` function for a graceful restart.

### 3.2. Bluetooth Low Energy (BLE) for Provisioning

**Rationale:** While the captive portal is a universal method for WiFi provisioning, it requires the user to disconnect from their current WiFi network, connect to the device's AP, and then reconnect later. A more modern and seamless user experience can be achieved using BLE. The device can advertise itself via BLE, and a companion app (or a web app using Web Bluetooth) can securely send WiFi credentials to the device without interrupting the user's existing network connection.

**Existing Work:** The codebase already includes `include/improv.h` and `include/improvserial.h`, which is an existing standard for BLE provisioning. This provides a strong foundation to build upon.

**Proposed Work:**
1.  Add a feature flag `ENABLE_BLE_PROVISIONING`.
2.  Implement the necessary BLE service based on the Improv protocol.
3.  Integrate the received credentials into the existing `WriteWiFiConfig` system, using `WifiCredSource::ImprovCreds`.
4.  Ensure that the main network loop correctly prioritizes these credentials.
