from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'PC'))
from protocol import Decoder, keys_packet, volume_packet, valid_state

class ProtocolTests(unittest.TestCase):
    def test_packets(self):
        self.assertEqual(keys_packet(5),b'K,05\n')
        self.assertEqual(volume_packet(0),b'VOL,0\n')
        for bad in (-1,128,'x'):
            with self.assertRaises(ValueError): keys_packet(bad)
        for bad in (-1,101,'2'):
            with self.assertRaises(ValueError): volume_packet(bad)

    def test_fragmentation_and_recovery(self):
        d=Decoder()
        self.assertEqual(d.feed(b'{"type":"po'),[])
        self.assertEqual(d.feed(b'ng"}\r\n'),[{'type':'pong'}])
        self.assertEqual(d.feed(b'x'*2048+b'\n{"type":"pong"}\n'),[{'type':'pong'}])
        self.assertEqual(d.feed(b'not-json\n[]\n{"type":"unknown"}\n'),[])
        self.assertEqual(len(d.buffer),0)

    def test_state_validation(self):
        state=dict(type='state',state='playing',score=10,best=20,lives=3,level=1,volume=20,pc=1,audio=1)
        self.assertTrue(valid_state(state))
        self.assertFalse(valid_state(dict(state,volume=101)))
        self.assertFalse(valid_state(dict(state,lives='3')))
        self.assertFalse(valid_state(dict(state,score=True)))

if __name__=='__main__': unittest.main()
