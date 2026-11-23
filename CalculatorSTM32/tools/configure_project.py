"""Create Keil metadata and enable peripherals in the laboratory HAL config."""
from pathlib import Path
import copy
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT.parent / 'Template_MC2025/Template_MC2025/Template_GPIO'
MODULES = ['DAC', 'DMA2D', 'SDRAM', 'LTDC', 'SPI', 'TIM', 'I2C']

def main():
    conf = ROOT / 'Firmware/Inc/stm32f4xx_hal_conf.h'
    text = conf.read_text()
    for module in MODULES:
        text = text.replace(f'/* #define HAL_{module}_MODULE_ENABLED */', f'#define HAL_{module}_MODULE_ENABLED')
    conf.write_text(text)
    startup = ROOT / 'MDK-ARM/startup_stm32f429xx.s'
    startup_text=startup.read_text().replace('0x400\n', '0x2000\n')
    startup_text=startup_text.replace('Heap_Size      EQU     0x200','Heap_Size      EQU     0x4000')
    startup.write_text(startup_text)
    tree = ET.parse(TEMPLATE / 'MDK-ARM/Template_GPIO.uvprojx')
    root = tree.getroot()
    target = root.find('Targets/Target')
    target.find('TargetName').text = 'CalculatorSTM32'
    target.find('uAC6').text = '1'
    target.find('pCCUsed').text = '6190000::V6.19::ARMCLANG'
    common = target.find('TargetOption/TargetCommonOption')
    common.find('Cpu').text = common.findtext('Cpu').replace('25000000', '180000000')
    common.find('OutputDirectory').text = '../build/keil/'
    common.find('OutputName').text = 'CalculatorSTM32'
    common.find('ListingPath').text = '../build/keil/'
    common.find('AfterMake/RunUserProg2').text = '0'
    cads = target.find('TargetOption/TargetArmAds/Cads')
    cads.find('VariousControls/IncludePath').text = ';'.join('../' + d for d in include_dirs())
    cads.find('VariousControls/Define').text = 'USE_HAL_DRIVER,STM32F429xx'
    cads.find('VariousControls/MiscControls').text = '-std=c99 -ffunction-sections'
    cads.find('Optim').text = '2'
    groups = target.find('Groups')
    groups.clear()
    grouped = {
        'Application': sorted((ROOT / 'Firmware/Src').glob('*.c')),
        'HAL': sorted((ROOT / 'Drivers/STM32F4xx_HAL_Driver/Src').glob('*.c')),
        'BSP': [ROOT / 'Drivers/BSP/STM32F429I-Discovery' / f for f in ['stm32f429i_discovery_lcd.c', 'stm32f429i_discovery_sdram.c']],
        'Components': [ROOT / 'Drivers/BSP/Components/ili9341/ili9341.c', ROOT / 'Drivers/BSP/Components/stmpe811/stmpe811.c'],
        'Startup': [ROOT / 'MDK-ARM/startup_stm32f429xx.s'],
    }
    for name, files in grouped.items():
        group = ET.SubElement(groups, 'Group')
        ET.SubElement(group, 'GroupName').text = name
        fs = ET.SubElement(group, 'Files')
        for path in files:
            entry = ET.SubElement(fs, 'File')
            ET.SubElement(entry, 'FileName').text = path.name
            ET.SubElement(entry, 'FileType').text = '2' if path.suffix == '.s' else '1'
            ET.SubElement(entry, 'FilePath').text = '../' + path.relative_to(ROOT).as_posix()
    # The original RTOS RTE metadata is not part of this bare-metal application.
    for rte in list(root.findall('RTE')):
        root.remove(rte)
    ET.indent(root, space='  ')
    tree.write(ROOT / 'MDK-ARM/CalculatorSTM32.uvprojx', encoding='utf-8', xml_declaration=True)
    print('Configured HAL and MDK-ARM/CalculatorSTM32.uvprojx')

def include_dirs():
    return ['Firmware/Inc', 'Drivers/STM32F4xx_HAL_Driver/Inc', 'Drivers/STM32F4xx_HAL_Driver/Inc/Legacy',
            'Drivers/CMSIS/Include', 'Drivers/CMSIS/Device/ST/STM32F4xx/Include',
            'Drivers/BSP/STM32F429I-Discovery', 'Drivers/BSP/Components/stmpe811', 'Utilities/Fonts']

if __name__ == '__main__':
    main()
