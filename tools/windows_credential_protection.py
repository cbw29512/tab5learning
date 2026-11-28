"""User-scoped Windows DPAPI adapter for bounded credential records."""
import ctypes
import logging
import sys
from ctypes import wintypes

LOG = logging.getLogger(__name__)
MAX_BLOB = 4096
UI_FORBIDDEN = 1


class Blob(ctypes.Structure):
    _fields_ = [("cbData", wintypes.DWORD),
                ("pbData", ctypes.POINTER(ctypes.c_ubyte))]


class WindowsProtector:
    def __init__(self):
        if sys.platform != "win32":
            raise OSError("Windows DPAPI is unavailable")
        self.crypt = ctypes.WinDLL("crypt32", use_last_error=True)
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        ptr = ctypes.POINTER(Blob)
        self.crypt.CryptProtectData.argtypes = [
            ptr, wintypes.LPCWSTR, ptr, ctypes.c_void_p, ctypes.c_void_p,
            wintypes.DWORD, ptr]
        self.crypt.CryptUnprotectData.argtypes = [
            ptr, ctypes.c_void_p, ptr, ctypes.c_void_p, ctypes.c_void_p,
            wintypes.DWORD, ptr]
        self.crypt.CryptProtectData.restype = wintypes.BOOL
        self.crypt.CryptUnprotectData.restype = wintypes.BOOL
        self.kernel.LocalFree.argtypes = [ctypes.c_void_p]
        self.kernel.LocalFree.restype = ctypes.c_void_p

    def _transform(self, data, entropy, protect):
        if not isinstance(data, bytes) or not 0 < len(data) <= MAX_BLOB:
            raise ValueError("Invalid DPAPI input size")
        if not isinstance(entropy, bytes) or not 0 < len(entropy) <= 256:
            raise ValueError("Invalid DPAPI context")
        source = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
        extra = (ctypes.c_ubyte * len(entropy)).from_buffer_copy(entropy)
        input_blob = Blob(len(data), source)
        entropy_blob = Blob(len(entropy), extra)
        output = Blob()
        try:
            operation = (self.crypt.CryptProtectData if protect
                         else self.crypt.CryptUnprotectData)
            if not operation(ctypes.byref(input_blob), None,
                             ctypes.byref(entropy_blob), None, None,
                             UI_FORBIDDEN, ctypes.byref(output)):
                raise OSError("Windows credential protection operation failed")
            if not output.pbData or not 0 < output.cbData <= MAX_BLOB:
                raise ValueError("Invalid DPAPI output size")
            return ctypes.string_at(output.pbData, output.cbData)
        except (OSError, ValueError):
            LOG.warning("DPAPI credential operation rejected")
            raise
        finally:
            # Clear native scratch buffers. Python bytes cannot promise erasure.
            ctypes.memset(source, 0, len(data))
            ctypes.memset(extra, 0, len(entropy))
            if output.pbData:
                if 0 < output.cbData <= MAX_BLOB:
                    ctypes.memset(output.pbData, 0, output.cbData)
                self.kernel.LocalFree(ctypes.cast(output.pbData, ctypes.c_void_p))

    def protect(self, data, entropy):
        return self._transform(data, entropy, True)

    def unprotect(self, data, entropy):
        return self._transform(data, entropy, False)
