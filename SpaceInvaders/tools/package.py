"""Audit project references, vendor hashes and HEX boot vectors; make a compact ZIP."""
from pathlib import Path
import hashlib
import json
import struct
import xml.etree.ElementTree as ET
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def audit_hex(path):
    data={};upper=0;eof=False
    for line in path.read_text().splitlines():
        assert line.startswith(':'),path
        record=bytes.fromhex(line[1:])
        assert sum(record)&255==0, 'HEX checksum'
        size,address,kind=record[0],int.from_bytes(record[1:3],'big'),record[3]
        assert len(record)==size+5, 'HEX record size'
        payload=record[4:-1]
        if kind==0:
            for i,value in enumerate(payload): data[upper+address+i]=value
        elif kind==4: upper=int.from_bytes(payload,'big')<<16
        elif kind==1: eof=True
    assert eof and min(data)==0x08000000 and max(data)<0x08200000
    def word(index): return struct.unpack('<I',bytes(data[0x08000000+index*4+i] for i in range(4)))[0]
    assert word(0)==0x20030000, 'Unexpected stack address'
    assert word(1)&1 and 0x08000000<=word(1)<0x08200000, 'Invalid reset vector'
    # SysTick, DMA1 Stream6, USART1 and DMA2 Stream0 point into Flash.
    for irq in [-1,17,37,56]:
        handler=word(16+irq)
        assert handler&1 and 0x08000000<=handler<0x08200000
    return {'file':str(path.relative_to(ROOT)), 'flash_bytes_in_hex':len(data), 'reset_vector':hex(word(1))}

def main():
    lock=json.loads((ROOT/'vendor-lock.json').read_text())
    for item in lock['files']:
        assert hashlib.sha256((ROOT/item['path']).read_bytes()).hexdigest()==item['sha256'],item['path']
    project=ROOT/'MDK-ARM/SpaceInvaders.uvprojx'
    tree=ET.parse(project)
    files=tree.findall('.//Groups/Group/Files/File/FilePath')
    for entry in files:
        assert (project.parent/entry.text).resolve().is_file(),entry.text
    hex_reports=[audit_hex(ROOT/'build'/name/(name+'.hex')) for name in ['SpaceInvaders','SpaceInvaders-no-audio']]
    manifest=[]
    build_allowed={'.hex','.bin','.elf','.json','.dll'}
    for path in sorted(ROOT.rglob('*')):
        if not path.is_file(): continue
        rel=path.relative_to(ROOT)
        if '.tools' in rel.parts or '__pycache__' in rel.parts or path.name=='delivery-manifest.json': continue
        if rel.parts[0]=='build' and path.suffix not in build_allowed: continue
        manifest.append({'path':rel.as_posix(),'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    report={'keil_source_paths_checked':len(files), 'downloaded_vendor_files_verified':len(lock['files']),
            'firmware':hex_reports,'hardware_tested':False,'files':manifest}
    manifest_path=ROOT/'delivery-manifest.json'
    manifest_path.write_text(json.dumps(report,indent=2)+'\n')
    archive=ROOT.parent/'SpaceInvaders.zip'
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for item in manifest: z.write(ROOT/item['path'],'SpaceInvaders/'+item['path'])
        z.write(manifest_path,'SpaceInvaders/delivery-manifest.json')
    with zipfile.ZipFile(archive) as z: assert z.testzip() is None
    print(json.dumps({'archive':str(archive),'zip_bytes':archive.stat().st_size,
                      'files':len(manifest)+1,'keil_paths':len(files),'vendor_files':len(lock['files']),
                      'firmware':hex_reports},indent=2))

if __name__=='__main__': main()
