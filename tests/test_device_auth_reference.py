import unittest
from tools.device_auth_reference import HostChallenge, proof


class DeviceAuthTests(unittest.TestCase):
    def test_mutual_proof_reference(self):
        key, device_nonce = b"k"*32, b"n"*32
        attempt = HostChallenge.begin("tab5-001", clock=lambda: 0)
        device = proof(key, "device", "tab5-001", attempt.host_nonce, device_nonce)
        host = attempt.complete(key, device_nonce, device, clock=lambda: 1)
        self.assertEqual(host, proof(key, "host", "tab5-001", attempt.host_nonce, device_nonce))
        self.assertNotEqual(host, device)
        with self.assertRaises(PermissionError):
            attempt.complete(key, device_nonce, device, clock=lambda: 2)

    def test_wrong_identity_key_role_and_expiration(self):
        key, nonce = b"k"*32, b"n"*32
        for wrong_key, role, device, now in [
            (b"x"*32, "device", "tab5-001", 1),
            (key, "host", "tab5-001", 1),
            (key, "device", "tab5-002", 1),
            (key, "device", "tab5-001", 11),
        ]:
            attempt = HostChallenge.begin("tab5-001", clock=lambda: 0)
            invalid = proof(wrong_key, role, device, attempt.host_nonce, nonce)
            with self.assertRaises(PermissionError):
                attempt.complete(key, nonce, invalid, clock=lambda: now)
            self.assertTrue(attempt.consumed)

    def test_fresh_challenges_and_bad_nonce(self):
        first = HostChallenge.begin("tab5-001")
        second = HostChallenge.begin("tab5-001")
        self.assertNotEqual(first.host_nonce, second.host_nonce)
        with self.assertRaises(ValueError):
            proof(b"k"*32, "device", "tab5-001", b"x", b"n"*32)
