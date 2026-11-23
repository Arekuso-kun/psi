"""Export actual C LCD frames; never archives the project."""
from pathlib import Path
import struct
import sys
import zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'PC'))
from controller import Simulator
from protocol import key_packet

def png(path,width,height,rgb):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+rgb[y*width*3:(y+1)*width*3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
                     +chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def main():
    sim=Simulator();out=ROOT/'docs/images';out.mkdir(parents=True,exist_ok=True)
    for key in '12+3*4=':sim.send(key_packet(key));sim.step(0,16)
    calc=sim.image().split(b'\n',3)[3];png(out/'calculator.png',240,320,calc)
    sim.send(key_packet('SET'));sim.step(0,16)
    settings=sim.image().split(b'\n',3)[3];png(out/'settings.png',240,320,settings)
    sim.send(key_packet('CAL'));sim.step(0,16)
    cal=sim.image().split(b'\n',3)[3];png(out/'calibration.png',240,320,cal)
    both=b''.join(calc[y*720:(y+1)*720]+settings[y*720:(y+1)*720] for y in range(320))
    png(out/'preview.png',480,320,both)
    print('Saved exact C framebuffer previews:',out)

if __name__=='__main__':main()
