# Surse și licențe

Codul aplicației este sub licența MIT. Componentele ST și Arm păstrează licențele și antetele originale. Sursele platformei au fost copiate din proiectul local SpaceInvaders, apoi aplicația a fost adaptată pentru calculator; nu există o dependență de sursele SpaceInvaders la rulare sau compilare.

- HAL STM32F4 v1.8.5 și CMSIS: din template-ul laboratorului, completate cu drivere oficiale din aceeași versiune. Licență HAL: `Drivers/STM32F4xx_HAL_Driver/LICENSE.txt`.
- LCD și SDRAM: [ST 32f429idiscovery-bsp](https://github.com/STMicroelectronics/32f429idiscovery-bsp). Licență în directorul BSP.
- ILI9341: [ST stm32-ili9341](https://github.com/STMicroelectronics/stm32-ili9341). Licență în directorul componentei.
- Touch: [ST stm32-stmpe811](https://github.com/STMicroelectronics/stm32-stmpe811), la commit-ul folosit de STM32CubeF4. Licență în directorul componentei.
- Interfețe `lcd.h`, `io.h`, `ts.h`: [ST stm32-bsp-common](https://github.com/STMicroelectronics/stm32-bsp-common), versiune compatibilă cu STM32CubeF4; licențe în antete.
- Fonturi BSP: [ST STM32CubeF4 Utilities/Fonts](https://github.com/STMicroelectronics/STM32CubeF4/tree/master/Utilities/Fonts). Interfața calculatorului utilizează fontul bitmap propriu din graphics.c.
- Startup GNU: [ST cmsis-device-f4](https://github.com/STMicroelectronics/cmsis-device-f4). Startup-ul Keil și configurația inițială provin din template-ul utilizatorului.
- Transport serial Python: [pySerial](https://github.com/pyserial/pyserial), instalat separat, licență BSD. Tkinter face parte din distribuția standard Python pentru Windows.

Commit-urile, URL-urile și hash-urile SHA256 pentru descărcările ST sunt în `vendor-lock.json`. Driverele descărcate nu au fost modificate. Compilatorul este extern; binarele includ runtime GNU/newlib, cu licențele și excepțiile aferente publicate în [distribuția Arm GNU 13.3.Rel1](https://support.arm.com/documentation/109845/13-3-Rel1).
