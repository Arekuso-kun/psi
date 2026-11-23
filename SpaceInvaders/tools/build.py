"""Build Cortex-M4 firmware or the native game tests/simulator without an IDE."""
from pathlib import Path
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
from configure_project import include_dirs

ROOT = Path(__file__).resolve().parents[1]

def run(args):
    result = subprocess.run([str(x) for x in args], cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(' '.join(map(str, args)) + '\n' + result.stdout + result.stderr)
    if result.stdout.strip(): print(result.stdout.strip())
    if result.stderr.strip(): print(result.stderr.strip())

def host():
    out = ROOT / 'build/host'
    out.mkdir(parents=True, exist_ok=True)
    cc = shutil.which('gcc')
    if not cc: raise SystemExit('Host GCC is required for --host.')
    os.environ['PATH'] = str(Path(cc).parent) + os.pathsep + os.environ.get('PATH', '')
    common = [cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-O2', '-I', ROOT / 'Firmware/Inc']
    core = [ROOT / 'Firmware/Src' / f for f in ['game.c', 'input.c', 'render.c']]
    exe = out / 'test_game.exe'
    run(common + core + [ROOT / 'tests/test_game.c', '-static', '-o', exe])
    run([exe])
    run(common + ['-shared', '-static-libgcc'] + core + [ROOT / 'tests/simulator_api.c', '-o', out / 'spaceinvaders_sim.dll'])
    print('Native tests and simulator DLL built.')

def firmware(toolchain, no_audio):
    if toolchain:
        cc = Path(toolchain) / 'arm-none-eabi-gcc.exe'
    else:
        found = shutil.which('arm-none-eabi-gcc')
        matches = list((ROOT / '.tools').glob('*/bin/arm-none-eabi-gcc.exe'))
        cc = Path(found) if found else matches[0] if matches else None
    if not cc or not cc.exists(): raise SystemExit('Run tools/get_toolchain.py or pass --toolchain PATH_TO_BIN.')
    name = 'SpaceInvaders-no-audio' if no_audio else 'SpaceInvaders'
    out = ROOT / 'build' / name
    out.mkdir(parents=True, exist_ok=True)
    sources = sorted((ROOT / 'Firmware/Src').glob('*.c'))
    sources += sorted((ROOT / 'Drivers/STM32F4xx_HAL_Driver/Src').glob('*.c'))
    sources += [ROOT / 'Drivers/BSP/STM32F429I-Discovery' / f for f in ['stm32f429i_discovery_lcd.c', 'stm32f429i_discovery_sdram.c']]
    sources += [ROOT / 'Drivers/BSP/Components/ili9341/ili9341.c', ROOT / 'Firmware/startup_stm32f429xx.s']
    arch = ['-mcpu=cortex-m4', '-mthumb', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard']
    flags = arch + ['-std=c99', '-Os', '-g3', '-ffunction-sections', '-fdata-sections', '-fno-common', '-DUSE_HAL_DRIVER', '-DSTM32F429xx', '-Wall', '-Wextra']
    if no_audio: flags += ['-DSI_AUDIO_ENABLED=0']
    for include in include_dirs(): flags += ['-I', str(ROOT / include)]
    objects = [out / (s.stem + '.o') for s in sources]
    def compile_one(pair):
        source, obj = pair
        own = source.parent == ROOT / 'Firmware/Src' and source.name != 'system_stm32f4xx.c'
        run([cc] + flags + (['-Werror'] if own else ['-Wno-unused-parameter']) + ['-c', source, '-o', obj])
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        list(pool.map(compile_one, zip(sources, objects)))
    elf = out / (name + '.elf')
    response = out / 'link.rsp'
    args = arch + ['--specs=nano.specs', '--specs=nosys.specs', '-T', str(ROOT / 'Firmware/STM32F429ZI_FLASH.ld'),
                   '-Wl,--gc-sections,-Map=' + str(out / (name + '.map')), '-o', str(elf)]
    args += [str(x) for x in objects] + ['-Wl,--start-group', '-lc', '-lm', '-lnosys', '-Wl,--end-group']
    response.write_text('\n'.join('"' + str(x).replace('\\', '/') + '"' for x in args))
    run([cc, '@' + str(response)])
    for fmt, suffix in [('ihex', '.hex'), ('binary', '.bin')]:
        run([cc.with_name('arm-none-eabi-objcopy.exe'), '-O', fmt, elf, out / (name + suffix)])
    run([cc.with_name('arm-none-eabi-size.exe'), elf])
    result = subprocess.run([str(cc), '--version'],capture_output=True,text=True,check=True)
    (out / 'build-info.json').write_text(json.dumps({'compiler':result.stdout.splitlines()[0], 'audio':not no_audio,
                                                    'hardware_tested':False, 'sources':[str(s.relative_to(ROOT)) for s in sources]},indent=2))
    print('Firmware:', elf)

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('--host', action='store_true')
    parser.add_argument('--no-audio', action='store_true')
    parser.add_argument('--toolchain', help='Directory containing arm-none-eabi-gcc.exe')
    args=parser.parse_args()
    if args.host: host()
    else: firmware(args.toolchain, args.no_audio)
