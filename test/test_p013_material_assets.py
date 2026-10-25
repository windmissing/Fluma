import hashlib
import tempfile
import unittest
from pathlib import Path
from tools.p013_prepare_material_assets import VERSIONS, load_assets
class MaterialAssetTests(unittest.TestCase):
    def fixture(self, root):
        meta = dict(VERSIONS, files={})
        for kind, width in [('grain',256),('strata',64)]:
            layers=[]
            for i, phase in enumerate(['H','L','M']):
                raw=bytes([128+i,128,128,255])*(width*256)
                name=kind+phase+'.rgba';(root/name).write_bytes(raw)
                meta['files'][kind+phase]=name;layers.append(raw)
            meta[kind+'Hash']=hashlib.sha256(b''.join(layers)).hexdigest()
        return meta
    def test_deterministic_layer_order_and_hashes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);m=self.fixture(root);a=load_assets(root,m);b=load_assets(root,m)
            self.assertEqual(a,b);self.assertEqual(len(a['grain']),786432)
            self.assertEqual(a['grain'][256*256*4],129)
    def test_missing_corrupt_alpha_hash_and_version_reject(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);m=self.fixture(root)
            m['grainHash']='wrong'
            with self.assertRaisesRegex(ValueError,'HASH'):load_assets(root,m)
            m=self.fixture(root);(root/'strataH.rgba').write_bytes(b'bad')
            with self.assertRaisesRegex(ValueError,'LENGTH'):load_assets(root,m)
            m=self.fixture(root);del m['files']['grainH']
            with self.assertRaisesRegex(ValueError,'MISSING'):load_assets(root,m)
            m=self.fixture(root);m['T_dataLayoutVersion']=1
            with self.assertRaisesRegex(ValueError,'VERSION'):load_assets(root,m)
    def test_outside_root_and_missing_file_reject(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);m=self.fixture(root);m['files']['grainH']='../outside.rgba'
            with self.assertRaisesRegex(ValueError,'PATH'):load_assets(root,m)
            m=self.fixture(root);(root/'grainH.rgba').unlink()
            with self.assertRaises(FileNotFoundError):load_assets(root,m)
if __name__=='__main__':unittest.main()
