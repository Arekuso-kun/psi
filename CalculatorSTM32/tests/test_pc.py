from pathlib import Path
import json
import random
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'PC'))
from protocol import Decoder,key_packet,nav_packet,volume_packet,valid_state,hit
from controller import Simulator

class ProtocolTests(unittest.TestCase):
    def test_packets_and_rejections(self):
        self.assertEqual(key_packet('7'),b'KEY,7\n');self.assertEqual(key_packet(','),b'KEY,.\n')
        self.assertEqual(nav_packet('R'),b'NAV,R\n');self.assertEqual(volume_packet(100),b'VOL,100\n')
        for value in ('1+2','7\nKEY,C','eval',''):
            with self.assertRaises(ValueError):key_packet(value)
        for value in (-1,101,True,'30'):
            with self.assertRaises(ValueError):volume_packet(value)

    def test_fragmentation_and_resync(self):
        d=Decoder();self.assertEqual(d.feed(b'{"ty'),[])
        self.assertEqual(d.feed(b'pe":"pong"}\r\n'),[{'type':'pong'}])
        self.assertEqual(d.feed(b'x'*2048+b'\n{"type":"pong"}\n'),[{'type':'pong'}])
        self.assertEqual(d.feed(b'not-json\n[]\n{"type":"unknown"}\n'),[])

    def test_state_and_hit_regions(self):
        s=dict(type='state',page='calculator',expression='1+2',result='3',error=0,selected=18,volume=20,
               pc=1,touch=1,calibrated=0,cal_step=0,audio=1)
        self.assertTrue(valid_state(s));self.assertFalse(valid_state(dict(s,volume=101)))
        self.assertFalse(valid_state(dict(s,error=True)));self.assertFalse(valid_state(dict(s,expression='"bad"')))
        self.assertEqual(hit('calculator',140,280),'=');self.assertEqual(hit('calculator',205,280),'=')
        self.assertEqual(hit('settings',140,120),'VOL+');self.assertIsNone(hit('calculator',0,0))

class IntegrationTests(unittest.TestCase):
    def setUp(self):self.sim=Simulator()
    def command(self,text):self.sim.send(text);self.sim.step(0,16)
    def test_uart_calculator_and_settings(self):
        for token in '2+3*4=':self.command(key_packet(token))
        s=self.sim.status();self.assertTrue(valid_state(s));self.assertEqual(s['result'],'14')
        self.command(key_packet('SET'));self.assertEqual(self.sim.status()['page'],'settings')
        self.command(volume_packet(70));self.assertEqual(self.sim.status()['volume'],70)
        self.command(key_packet('MUTE'));self.assertEqual(self.sim.status()['volume'],0)
        self.command(key_packet('MUTE'));self.assertEqual(self.sim.status()['volume'],70)
        self.command(key_packet('BACK'));self.assertEqual(self.sim.status()['expression'],'2+3*4')
    def test_touch_joystick_keyboard_in_same_expression(self):
        self.sim.touch(36,245,1);self.sim.step(0,16);self.sim.touch(36,245,0);self.sim.step(0,16)
        self.command(key_packet('+'))
        # The touch selected digit 1 (index 12); physical joystick right selects digit 2.
        self.sim.step(2,1);self.sim.step(2,21);self.sim.step(0,1);self.sim.step(0,21)
        self.sim.step(16,1);self.sim.step(16,21);self.sim.step(0,1);self.sim.step(0,21)
        self.command(key_packet('='));self.assertEqual(self.sim.status()['result'],'3')
    def test_many_expressions(self):
        rng=random.Random(42)
        for _ in range(500):
            a,b,c,d=(rng.randrange(1000) for _ in range(4));e=rng.randrange(1,100)
            self.command(key_packet('C'))
            for token in f'{a}+{b}*{c}-{d}/{e}=':self.command(key_packet(token))
            s=self.sim.status();self.assertEqual(s['error'],0)
            expected=a+b*c-d/e
            self.assertAlmostEqual(float(s['result']),expected,delta=max(1,abs(expected))*5e-10)

if __name__=='__main__':unittest.main()
