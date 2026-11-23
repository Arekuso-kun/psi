"""Verify source references, vendor hashes and HEX files. No archiving or flashing."""
from pathlib import Path
import hashlib
import json
import struct
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
def check_hex(path):
    data={};upper=0;eof=False
    for line in path.read_text().splitlines():
        assert line.startswith(':'),path
        record=bytes.fromhex(line[1:]);assert sum(record)&255==0,'HEX checksum'
        count=record[0];address=int.from_bytes(record[1:3],'big');kind=record[3];payload=record[4:-1]
        assert len(record)==count+5,'HEX record length'
        if kind==0:
            for i,byte in enumerate(payload):data[upper+address+i]=byte
        elif kind==4:upper=int.from_bytes(payload,'big')<<16
        elif kind==1:eof=True
    assert eof and min(data)==0x08000000 and max(data)<0x08200000
    def word(index):return struct.unpack('<I',bytes(data[0x08000000+index*4+i] for i in range(4)))[0]
    assert word(0)==0x20030000
    for index in [1,15,16+17,16+37,16+56]:
        assert word(index)&1 and 0x08000000<=word(index)<0x08200000
    return {'path':str(path.relative_to(ROOT)),'flash_bytes':len(data),'reset_vector':hex(word(1))}

def main():
    lock=json.loads((ROOT/'vendor-lock.json').read_text())
    for item in lock['files']:
        assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest()==item['sha256'],item['path']
    project=ROOT/'MDK-ARM/CalculatorSTM32.uvprojx';paths=ET.parse(project).findall('.//Groups/Group/Files/File/FilePath')
    for p in paths:assert (project.parent/p.text).resolve().is_file(),p.text
    reports=[check_hex(ROOT/'build'/name/(name+'.hex')) for name in ['CalculatorSTM32','CalculatorSTM32-no-audio']]
    assert not (ROOT.parent/'CalculatorSTM32.zip').exists(),'Unexpected calculator archive'
    assert not list(ROOT.rglob('*.zip')),'Unexpected archive inside calculator project'
    report={'vendor_files_verified':len(lock['files']),'keil_references_checked':len(paths),'firmware':reports,
            'hardware_tested':False,'keil_compilation_tested':False,'archived':False}
    (ROOT/'build/validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
