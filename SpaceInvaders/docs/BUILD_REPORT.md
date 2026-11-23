# Rezultatul verificării software

Data: 16 noiembrie 2026.

- Compilator firmware: Arm GNU Toolchain 13.3.Rel1, GCC 13.3.1, Cortex-M4F, Thumb, ABI hard-float, optimizare pentru dimensiune.
- Varianta cu audio: compilare și link reușite, fără avertismente în build-ul final; `text=27540`, `data=172`, `bss=12500` octeți conform `arm-none-eabi-size`. Zona `bss` raportată include rezerva de heap și stack.
- Varianta fără audio: compilare și link reușite, fără avertismente în build-ul final; `text=25444`, `data=172`, `bss=11780` octeți.
- Framebufferele sunt în SDRAM externă și nu sunt incluse în ocuparea SRAM internă de mai sus.
- Testele native C au trecut, inclusiv rularea de 30.000 de pași. GCC host: MSYS2 UCRT64 16.1.0.
- Cele trei teste Python de protocol au trecut.
- Aplicația Tkinter a trecut proba de pornire atât cu simulator, cât și fără port serial conectat. Nu a fost trimisă nicio comandă către hardware real în timpul verificării.
- Previzualizările din `docs/images` sunt exportate din framebuffer-ul rendererului C, nu sunt fotografii ale LCD-ului fizic.
- Fișierele vendor sunt verificate prin SHA256, referințele de surse din proiectul Keil sunt verificate pentru existență, iar HEX-urile sunt verificate pentru checksum și vectori de boot de către `tools/package.py`.

**Rămân de verificat:** compilarea în Keil, programarea plăcii, recablarea joystick-ului, LCD/SDRAM pe hardware, legătura UART reală, identificarea modulului audio și nivelul sonor. Fișierele generate sunt o bază implementată și compilată, nu o certificare că montajul fizic funcționează deja.
