"""Download a portable compiler into this workspace; does not install globally."""
from pathlib import Path
import hashlib
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
NAME = 'arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi'
URL = f'https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.3.rel1/binrel/{NAME}.zip'

def main():
    dest = ROOT / '.tools'
    dest.mkdir(exist_ok=True)
    archive = dest / (NAME + '.zip')
    if not archive.exists():
        print('Downloading portable Arm GNU Toolchain 13.3.Rel1...', flush=True)
        urllib.request.urlretrieve(URL, archive)
    print('Archive SHA256:', hashlib.sha256(archive.read_bytes()).hexdigest(), flush=True)
    with zipfile.ZipFile(archive) as z:
        base = dest.resolve()
        for member in z.infolist():
            if not (base / member.filename).resolve().is_relative_to(base):
                raise ValueError('Unsafe archive path')
        z.extractall(dest)
    print(dest / NAME / 'bin/arm-none-eabi-gcc.exe', flush=True)

if __name__ == '__main__':
    main()
