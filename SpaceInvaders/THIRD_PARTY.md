# Third-party sources

STMicroelectronics components retain their original copyright headers and licenses.

- STM32F4 HAL v1.8.5: files supplied in the laboratory template, supplemented by the matching official [stm32f4xx-hal-driver](https://github.com/STMicroelectronics/stm32f4xx-hal-driver) release. License: `Drivers/STM32F4xx_HAL_Driver/LICENSE.txt`.
- CMSIS core and STM32F429 device headers: copied from the user's laboratory template. The source headers contain Arm/ST copyright and license notices.
- Board LCD and SDRAM support: [32f429idiscovery-bsp](https://github.com/STMicroelectronics/32f429idiscovery-bsp). License: `Drivers/BSP/STM32F429I-Discovery/LICENSE.md`.
- ILI9341 initialization: [stm32-ili9341](https://github.com/STMicroelectronics/stm32-ili9341). License: `Drivers/BSP/Components/ili9341/LICENSE.md`.
- Legacy LCD driver interface compatible with the STM32CubeF4 gitlink: [stm32-bsp-common](https://github.com/STMicroelectronics/stm32-bsp-common). License notice is embedded in `Drivers/BSP/Components/Common/lcd.h`.
- BSP fonts: [STM32CubeF4 Utilities/Fonts](https://github.com/STMicroelectronics/STM32CubeF4/tree/master/Utilities/Fonts). License notices are embedded in each file. The game's renderer uses its own compact font.
- GNU startup: [cmsis-device-f4](https://github.com/STMicroelectronics/cmsis-device-f4). Original ST headers are preserved.
- The Keil project metadata, system initialization source, HAL configuration and Keil startup were adapted from the user's `Template_MC2025` project.
- PC serial transport requires [pySerial](https://github.com/pyserial/pyserial), installed separately under its BSD license. Tkinter is part of standard Python Windows distributions.

Exact downloaded source commits, URLs and file SHA256 digests are recorded in `vendor-lock.json`. Downloaded ST files are unmodified; application changes are in Firmware and project metadata. The compiler download stays under `.tools` and is not included in delivery ZIPs. Prebuilt firmware links the Arm GNU toolchain runtime under its applicable runtime licenses and exceptions; the GNU tools and runtime source/license information are available from the [Arm release](https://support.arm.com/documentation/109845/13-3-Rel1).
