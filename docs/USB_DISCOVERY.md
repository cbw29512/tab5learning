# Passive USB discovery — host prototype

The teacher host already uses pyserial for the separate ink diagnostic. The learner preview does not implement its PING protocol; never send diagnostic or provisioning commands to it.

## State and schema
Discovery is a transient snapshot: dependency unavailable / enumeration error / no candidates / candidates detected. Each candidate contains a COM port and VID/PID. The known physical prototype exposes Espressif USB Serial/JTAG 303A:1001. This is a generic ESP32 interface, not proof of Tab5 model, firmware, student assignment or authorization. Do not persist a serial number or match an assignment using VID/PID alone.

A scan is explicitly requested by an authenticated administrator in test mode, protected by the existing session and CSRF checks. Enumerate Windows ports without opening serial connections, resetting, reading data or sending commands. Filter to the known interface, cap displayed results, escape output. Errors are visible and distinguishable from an empty result. No actual student data, firmware edits or USB writes.

## Acceptance
Install existing optional Windows dependency, scan connected Tab5, verify expected COM5 appears, unplug and rescan to verify disappearance, reconnect and rescan. Confirm unrelated ports are excluded and no reset or screen interruption occurs. Unit tests inject fake port enumeration; no physical identity claim until an authenticated firmware handshake and device-specific credential are implemented. Keep assignments PLANNED until durable authenticated provisioning acknowledgment.
