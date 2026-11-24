import unittest
from zosi_lan import private_ip, rtsp_candidates, redact_url, probe_rtsp, scan_local_subnet

class ZosiLanTests(unittest.TestCase):
    def test_private_network(self):
        self.assertTrue(private_ip('192.168.1.9'))
        self.assertFalse(private_ip('8.8.8.8'))
        self.assertFalse(private_ip('127.0.0.1'))
    def test_candidates_and_escaping(self):
        urls=rtsp_candidates('192.168.2.9','admin','a@b:c')
        self.assertIn('/video1',urls[0]);self.assertIn('a%40b%3Ac',urls[0]);self.assertEqual(len(urls),4)
        self.assertNotIn('a%40b%3Ac',redact_url(urls[0]))
    def test_no_public_url_probes(self):
        self.assertFalse(probe_rtsp('rtsp://8.8.8.8/video1')['ok'])
    def test_public_scan_rejected(self):
        with self.assertRaises(ValueError): scan_local_subnet('8.8.8.0/24')
    def test_huge_scan_rejected(self):
        with self.assertRaises(ValueError): scan_local_subnet('192.168.0.0/16')

if __name__=='__main__':unittest.main()
