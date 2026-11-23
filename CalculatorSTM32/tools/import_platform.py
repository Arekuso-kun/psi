"""One-time bootstrap from the existing, tested platform and official ST drivers.
Do not rerun after editing platform files; all dependencies are delivered locally.
"""
from pathlib import Path
import hashlib
import json
import shutil
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent / 'SpaceInvaders'

def fetch(url):
    req = urllib.request.Request(url, headers={'User-Agent':'CalculatorSTM32-course-project'})
    with urllib.request.urlopen(req, timeout=60) as response:
        return response.read()

def main():
    if (ROOT / 'Firmware/Src/board.c').exists():
        raise SystemExit('Platform already initialized. Refusing to overwrite application files.')
    for folder in ['Drivers', 'Utilities']:
        shutil.copytree(SOURCE/folder, ROOT/folder, dirs_exist_ok=True)
    for folder in ['Firmware/Inc', 'Firmware/Src', 'MDK-ARM', 'PC', 'tests', 'docs/images']:
        (ROOT/folder).mkdir(parents=True, exist_ok=True)
    copies = ['Firmware/STM32F429ZI_FLASH.ld', 'Firmware/startup_stm32f429xx.s',
              'Firmware/Src/system_stm32f4xx.c', 'Firmware/Src/syscalls.c',
              'Firmware/Inc/stm32f4xx_hal_conf.h', 'MDK-ARM/startup_stm32f429xx.s', 'LICENSE']
    for file in copies:
        shutil.copy2(SOURCE/file, ROOT/file)
    for file in ['Firmware/Src/board.c','Firmware/Inc/board.h','Firmware/Inc/board_config.h',
                 'Firmware/Src/audio.c','Firmware/Inc/audio.h', 'tools/build.py','tools/configure_project.py']:
        import re
        text=(SOURCE/file).read_text().replace('SpaceInvaders','CalculatorSTM32')
        text=re.sub(r'\bSI_', 'MC_', text)
        text=re.sub(r'\bsi_', 'mc_', text)
        (ROOT/file).write_text(text)
    lock=json.loads((SOURCE/'vendor-lock.json').read_text())
    hal=lock['sources']['HAL']['commit']
    common=lock['sources']['COMMON']['commit']
    cube=lock['sources']['CUBE']['commit']
    info=json.loads(fetch(f'https://api.github.com/repos/STMicroelectronics/STM32CubeF4/contents/Drivers/BSP/Components/stmpe811?ref={cube}'))
    stmpe=info['sha']
    lock['sources']['STMPE811']={'repo':'stm32-stmpe811','commit':stmpe}
    files=[]
    for stem in ['i2c','i2c_ex']:
        for ext,folder in [('h','Inc'),('c','Src')]:
            files.append((f'https://raw.githubusercontent.com/STMicroelectronics/stm32f4xx-hal-driver/{hal}/{folder}/stm32f4xx_hal_{stem}.{ext}',
                          f'Drivers/STM32F4xx_HAL_Driver/{folder}/stm32f4xx_hal_{stem}.{ext}'))
    for name in ['stmpe811.c','stmpe811.h','LICENSE.md']:
        files.append((f'https://raw.githubusercontent.com/STMicroelectronics/stm32-stmpe811/{stmpe}/{name}',f'Drivers/BSP/Components/stmpe811/{name}'))
    for name in ['ts.h','io.h']:
        files.append((f'https://raw.githubusercontent.com/STMicroelectronics/stm32-bsp-common/{common}/{name}',f'Drivers/BSP/Components/Common/{name}'))
    for url,file in files:
        data=fetch(url)
        target=ROOT/file
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(data)
        lock['files'].append({'path':file,'source':url,'sha256':hashlib.sha256(data).hexdigest()})
    (ROOT/'vendor-lock.json').write_text(json.dumps(lock,indent=2)+'\n')
    print('Platform copied; downloaded',len(files),'official ST touch/I2C files.')

if __name__=='__main__':main()
