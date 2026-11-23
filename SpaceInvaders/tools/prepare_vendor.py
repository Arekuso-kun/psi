"""Prepare the STM32 project from the supplied template and official ST sources."""
from pathlib import Path
import concurrent.futures
import hashlib
import json
import shutil
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT.parent / 'Template_MC2025/Template_MC2025/Template_GPIO'
HEADERS = {'User-Agent': 'SpaceInvaders-course-project'}

def fetch(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=90) as r:
        return r.read()

def main():
    # Use the exact HAL version already supplied by the laboratory.
    sources = {
        'HAL': ('stm32f4xx-hal-driver', 'v1.8.5'),
        'BSP': ('32f429idiscovery-bsp', 'main'),
        'ILI9341': ('stm32-ili9341', 'main'),
        'COMMON': ('stm32-bsp-common', '1e18c5afdf1f5971a35c8e2f88b6a21e5568ed92'),
    }
    lock_file = ROOT / 'vendor-lock.json'
    saved = json.loads(lock_file.read_text())['sources'] if lock_file.exists() else {}
    locks = {}
    for name, (repo, ref) in sources.items():
        if name in saved:
            locks[name] = saved[name]
        else:
            info = json.loads(fetch(f'https://api.github.com/repos/STMicroelectronics/{repo}/commits/{ref}'))
            locks[name] = {'repo': repo, 'commit': info['sha']}
    files = []
    modules = ['ltdc', 'ltdc_ex', 'dma2d', 'sdram', 'spi', 'dac', 'dac_ex', 'tim', 'tim_ex']
    for module in modules:
        for folder, suffix in [('Inc', '.h'), ('Src', '.c')]:
            files.append(('HAL', f'{folder}/stm32f4xx_hal_{module}{suffix}', f'Drivers/STM32F4xx_HAL_Driver/{folder}/stm32f4xx_hal_{module}{suffix}'))
    files += [('HAL', 'Src/stm32f4xx_ll_fmc.c', 'Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_fmc.c'),
              ('HAL', 'Inc/stm32f4xx_ll_fmc.h', 'Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_ll_fmc.h')]
    for stem in ['stm32f429i_discovery', 'stm32f429i_discovery_lcd', 'stm32f429i_discovery_sdram']:
        for suffix in ['.c', '.h']:
            files.append(('BSP', stem + suffix, 'Drivers/BSP/STM32F429I-Discovery/' + stem + suffix))
    for file in ['ili9341.c', 'ili9341.h', 'LICENSE.md']:
        files.append(('ILI9341', file, 'Drivers/BSP/Components/ili9341/' + file))
    files.append(('BSP', 'LICENSE.md', 'Drivers/BSP/STM32F429I-Discovery/LICENSE.md'))
    sources['CMSIS_DEVICE'] = ('cmsis-device-f4', 'master')
    if 'CMSIS_DEVICE' in saved:
        locks['CMSIS_DEVICE'] = saved['CMSIS_DEVICE']
    else:
        info = json.loads(fetch('https://api.github.com/repos/STMicroelectronics/cmsis-device-f4/commits/master'))
        locks['CMSIS_DEVICE'] = {'repo': 'cmsis-device-f4', 'commit': info['sha']}
    files.append(('CMSIS_DEVICE', 'Source/Templates/gcc/startup_stm32f429xx.s', 'Firmware/startup_stm32f429xx.s'))
    # LCD component interface and fonts are supplied in STM32CubeF4 itself.
    if 'CUBE' in saved:
        locks['CUBE'] = saved['CUBE']
    else:
        cube_info = json.loads(fetch('https://api.github.com/repos/STMicroelectronics/STM32CubeF4/commits/master'))
        locks['CUBE'] = {'repo': 'STM32CubeF4', 'commit': cube_info['sha']}
    for file in ['lcd.h']:
        files.append(('COMMON', file, 'Drivers/BSP/Components/Common/' + file))
    for file in ['fonts.h', 'font24.c', 'font20.c', 'font16.c', 'font12.c', 'font8.c']:
        files.append(('CUBE', 'Utilities/Fonts/' + file, 'Utilities/Fonts/' + file))
    for folder in ['Inc', 'Src']:
        shutil.copytree(TEMPLATE / 'Drivers/STM32F4xx_HAL_Driver' / folder,
                        ROOT / 'Drivers/STM32F4xx_HAL_Driver' / folder, dirs_exist_ok=True)
    cmsis = ROOT / 'Drivers/CMSIS'
    shutil.copytree(TEMPLATE / 'Drivers/CMSIS/Include', cmsis / 'Include', dirs_exist_ok=True)
    shutil.copytree(TEMPLATE / 'Drivers/CMSIS/Device/ST/STM32F4xx/Include', cmsis / 'Device/ST/STM32F4xx/Include', dirs_exist_ok=True)
    shutil.copy2(TEMPLATE / 'Drivers/STM32F4xx_HAL_Driver/LICENSE.txt', ROOT / 'Drivers/STM32F4xx_HAL_Driver/LICENSE.txt')
    def one(item):
        key, source, target = item
        lock = locks[key]
        url = f"https://raw.githubusercontent.com/STMicroelectronics/{lock['repo']}/{lock['commit']}/{source}"
        try:
            data = fetch(url)
        except Exception as error:
            raise RuntimeError(f'Download failed: {url}') from error
        out = ROOT / target
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(data)
        return {'path': target, 'source': url, 'sha256': hashlib.sha256(data).hexdigest()}
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        manifest = list(pool.map(one, files))
    (ROOT / 'vendor-lock.json').write_text(json.dumps({'sources': locks, 'files': manifest}, indent=2) + '\n')
    (ROOT / 'Firmware/Src').mkdir(parents=True, exist_ok=True)
    shutil.copy2(TEMPLATE / 'Src/system_stm32f4xx.c', ROOT / 'Firmware/Src/system_stm32f4xx.c')
    shutil.copy2(TEMPLATE / 'Inc/stm32f4xx_hal_conf.h', ROOT / 'Firmware/Inc/stm32f4xx_hal_conf.h')
    (ROOT / 'MDK-ARM').mkdir(exist_ok=True)
    shutil.copy2(TEMPLATE / 'MDK-ARM/startup_stm32f429xx.s', ROOT / 'MDK-ARM/startup_stm32f429xx.s')
    print(f'Prepared {len(manifest)} official source files and local template headers.')

if __name__ == '__main__':
    main()
