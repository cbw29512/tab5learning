import unittest
import zlib
from tools.device_credential_record import encode_credential, decode_credential


class CredentialRecordTests(unittest.TestCase):
    def test_cross_language_vector(self):
        key = bytes(range(32))
        record = encode_credential("TAB5-001", key)
        self.assertEqual(len(record), 105)
        self.assertEqual(record[-4:], bytes.fromhex("c9a10ed5"))
        self.assertEqual(decode_credential(record), ("tab5-001", key))

    def test_corruption(self):
        record = encode_credential("tab5-001", bytes(range(32)))
        for index in range(len(record)):
            damaged = bytearray(record)
            damaged[index] ^= 1
            with self.assertRaises(ValueError):
                decode_credential(bytes(damaged))

    def test_invalid_id_and_key(self):
        for ident, key in [("a b", b"k"*32), ("a"*65, b"k"*32),
                           ("tab5", bytes(32)), ("tab5", b"short")]:
            with self.assertRaises(ValueError):
                encode_credential(ident, key)
        valid = encode_credential("a"*64, b"k"*32)
        self.assertEqual(decode_credential(valid)[0], "a"*64)

    def test_invalid_fields_with_valid_crc(self):
        body = bytearray(encode_credential("tab5", b"k"*32)[:-4])
        body[10] = 1
        with self.assertRaises(ValueError):
            decode_credential(bytes(body) + zlib.crc32(body).to_bytes(4, "little"))
