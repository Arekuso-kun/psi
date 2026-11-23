"""Export screenshots of the actual C renderer without extra Python packages."""
from pathlib import Path
import struct
import sys
import zlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'PC'))
from controller import Simulator

def png(path, width, height, rgb):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+rgb[y*width*3:(y+1)*width*3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))
                     +chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b''))

def main():
    sim=Simulator()
    out=ROOT/'docs/images';out.mkdir(parents=True,exist_ok=True)
    title=sim.image().split(b'\n',3)[3]
    png(out/'title.png',240,320,title)
    sim.step(64,0,16)
    for i in range(180): sim.step(4|(2 if i<70 else 1 if i<140 else 0),0,16)
    game=sim.image().split(b'\n',3)[3]
    png(out/'game.png',240,320,game)
    both=b''.join(title[y*720:(y+1)*720]+game[y*720:(y+1)*720] for y in range(320))
    png(out/'preview.png',480,320,both)
    print('Saved title/game preview from the C framebuffer:',out/'preview.png')

if __name__=='__main__': main()
