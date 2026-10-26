import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
spec = importlib.util.spec_from_file_location("capture", Path(__file__).parents[1] / "tools/p011_capture_manifest.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class CaptureManifestTest(unittest.TestCase):
    def test_raw_counts_hash_and_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            ledger, admissions = Path(directory)/"ledger", Path(directory)/"admissions"
            ledger.write_text(json.dumps({"context": {"runId":"r", "identityRevision":"P011_COMMIT_IDENTITY_V2","sequence":1,"frameId":2,"substepId":3,"attemptId":"a","sourceEpoch":4,"observationRouteId":"route"}})+"\n",encoding="utf-8")
            admissions.write_text(json.dumps({"runId":"r"})+"\n",encoding="utf-8")
            fence = dict(state="CLOSED",runId="r",identityRevision="P011_COMMIT_IDENTITY_V2",lastSequence=1,lastFrameId=2,lastSubstepId=3,lastAttemptId="a",lastSourceEpoch=4,routeId="route",sentRecords=1,writtenRecords=1,sentAdmissions=1,writtenAdmissions=1)
            result = module.manifest(ledger,admissions,fence)
            self.assertEqual(result["status"],"COMPLETE")
            self.assertEqual(result["ledger"]["bytes"],ledger.stat().st_size)
            self.assertEqual(len(result["ledger"]["sha256"]),64)
            for change in [dict(state="DRAINING"),dict(writtenAdmissions=0),dict(runId="other")]:
                self.assertEqual(module.manifest(ledger,admissions,{**fence,**change})["status"],"PREFIX_OR_MISMATCH")
            ledger.write_text(ledger.read_text()+ledger.read_text(),encoding="utf-8")
            self.assertEqual(module.manifest(ledger,admissions,fence)["ledger"]["rawRows"],2)
            self.assertEqual(module.manifest(ledger,admissions,fence)["status"],"PREFIX_OR_MISMATCH")

if __name__ == '__main__': unittest.main()
