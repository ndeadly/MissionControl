import importlib.machinery, importlib.util, tempfile, unittest
from pathlib import Path

class HealthScriptTests(unittest.TestCase):
    def test_health_source_has_required_controls(self):
        root=Path(__file__).resolve().parents[1]
        script=(root/'appliance/dynamicos-health').read_text()
        self.assertIn('age > 40',script)
        self.assertIn("status.get('state') != 'recording'",script)
        self.assertIn("status.get('running',0)",script)
        self.assertIn('crypt',script)
    def test_cache_limit_and_isolation(self):
        root=Path(__file__).resolve().parents[1]
        content=(root/'appliance/run-dynamicos\\x2dnvr-hls.mount').read_text()
        for item in ['size=512M','noexec','nosuid','nodev','Type=tmpfs']:
            self.assertIn(item,content)
    def test_mount_enabled(self):
        root=Path(__file__).resolve().parents[1]
        script=(root/'appliance/install-on-debian.sh').read_text()
        self.assertIn('run-dynamicos\\x2dnvr-hls.mount',script)
