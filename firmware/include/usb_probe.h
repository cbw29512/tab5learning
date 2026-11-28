#pragma once
#include "ink_capture.h"

// Read-only, unauthenticated development diagnostics only.
// Do not add student records or teacher update commands to this protocol.
void usb_probe_poll(const InkCapture& capture);
void usb_probe_export(const InkCapture& capture);
