# Space Invaders - STM32F429I-DISCO

Proiect pentru disciplina Microcontrolere: joc pe LCD-ul plăcii, comenzi locale și din PC disponibile simultan, sunet prin DAC și volum reglabil.

**Stare:** firmware compilat cu Arm GNU Toolchain; logica și protocolul verificate pe PC. Placa fizică, cablarea, sunetul și compilarea în Keil nu au fost testate în această sesiune. Nu s-a programat automat nicio placă. Template-ul original a rămas intact.

## Începe de aici

1. Citește [conexiunile hardware](docs/HARDWARE.md). Joystick-ul trebuie recablat înainte de a-l folosi împreună cu LCD-ul. Modulul audio trebuie identificat înainte de conectare.
2. Pentru probă pe PC, fără placă și fără pachete Python suplimentare, rulează `PC/Start_Demo.cmd` sau, din acest director:

   ```powershell
   python PC/controller.py --demo
   ```

   Simulatorul Windows x64 inclus folosește exact fișierele C `game.c`, `input.c` și `render.c` ale firmware-ului. Butoanele de pe ecran simulează joystick-ul și pot fi combinate cu tastatura. Simularea nu reproduce perifericele și nu redă sunet.

3. Pentru placa reală, programează prin ST-LINK unul dintre fișierele deja generate:

   - `build/SpaceInvaders/SpaceInvaders.hex`: joc și ieșire audio DAC pe PA5.
   - `build/SpaceInvaders-no-audio/SpaceInvaders-no-audio.hex`: prima probă, fără activarea ieșirii audio.

   Poți folosi STM32CubeProgrammer sau debuggerul Keil. Pentru un fișier `.bin`, adresa de încărcare este `0x08000000`; HEX-ul conține adresele. Verifică modelul STM32F429ZI înainte de programare.

4. Pentru tastatura PC-ului:

   ```powershell
   python -m pip install -r PC/requirements.txt
   python PC/controller.py
   ```

   Selectează COM-ul și apasă **Conectează**. Un port deschis nu garantează răspunsul plăcii: aplicația afișează „Placă conectată” după recepția stării jocului. Nu ține același COM deschis în Tera Term și în aplicație simultan.

## Comenzi

| Acțiune | Placă | PC |
|---|---|---|
| Start / restart după Game Over | USER sau apăsare centrală joystick | Space, Enter sau R |
| Deplasare | Joystick stânga/dreapta | A/D sau săgeți stânga/dreapta |
| Foc | USER ținut apăsat | Space ținut apăsat |
| Volum | Joystick sus/jos | + / - sau slider |
| Pauză/reluare | Apăsare centrală joystick | P |

Nu se selectează un mod de control. Ultima direcție nou apăsată are prioritate dacă cele două surse cer direcții opuse; când o eliberezi, cealaltă comandă încă apăsată se reia. Focul este combinat din ambele surse, cu o limită comună. La pierderea heartbeat-ului PC, numai comenzile PC expiră după 350 ms. Pierderea focusului aplicației eliberează tastele PC. Jocul local continuă.

Volumul are 0..100 de unități, inițial 20; 0 înseamnă mute. Valoarea este o setare de amplitudine, nu o măsurătoare în decibeli. Modificările prin joystick se văd și pe PC. Scorul maxim și volumul sunt păstrate în RAM, până la reset; nu se scriu în Flash.

## Compilare

### Keil µVision

Deschide `MDK-ARM/SpaceInvaders.uvprojx`. Proiectul este pregătit pentru Arm Compiler 6. În **Options for Target > Target**, selectează versiunea Arm Compiler 6 instalată; metadatele inițiale indică 6.19. Instalează/selectează Device Family Pack pentru STM32F429ZI dacă Keil îl cere. Alege ST-LINK pentru debug/download și algoritmul Flash pentru STM32F4 de 2 MB.

Toate sursele HAL, CMSIS și BSP necesare sunt incluse. Nu este nevoie să generezi din nou template-ul în CubeMX. Proiectul folosește inițializare manuală și BSP; un fișier `.ioc` nou nu a fost generat. Nu regenera template-ul original peste acest director.

Compilarea Keil trebuie verificată pe calculatorul cu Keil instalat. Artefactele binare incluse au fost produse cu GNU, nu cu Keil.

### GNU, verificat în această sesiune

Ai nevoie de Arm GNU Toolchain cu `arm-none-eabi-gcc` și Python 3.10+:

```powershell
python tools/build.py
python tools/build.py --no-audio
```

Scriptul găsește compilatorul din PATH sau din `.tools/*/bin`. Poți specifica explicit:

```powershell
python tools/build.py --toolchain "C:/cale/catre/arm-toolchain/bin"
```

Opțional, `python tools/get_toolchain.py` descarcă varianta Windows portabilă Arm GNU 13.3.Rel1 în `.tools`, fără instalare globală. Compilatorul nu este inclus în arhiva de livrare. Compilarea principală este Windows; scripturile utilizează nume de executabile `.exe`.

Sursele ST sunt deja incluse și fixate prin commit și SHA256 în `vendor-lock.json`. `prepare_vendor.py` este un instrument de reconstruire a dependențelor pornind de la template-ul original, nu o etapă normală de build. Dacă îl rulezi, rulează după aceea `configure_project.py` pentru a reactiva modulele HAL. Nu îl rula după modificări proprii în fișierele pe care le copiază.

### Teste și previzualizări

Cu GCC pentru Windows disponibil în PATH:

```powershell
python tools/build.py --host
python -m unittest discover -s tests -p "test_pc.py" -v
python PC/controller.py --demo --smoke-test
python tools/preview.py
```

Testele C acoperă combinarea comenzilor, revenirea la joystick după deconectare, expirarea la depășirea contorului de milisecunde, protocolul invalid, coliziunile, viețile, valurile, pauza, limitele volumului, limitele framebuffer-ului și o rulare cu 30.000 de pași. Testele Python verifică formarea mesajelor, fragmentarea recepției și recuperarea după mesaje invalide sau prea lungi.

## Arhitectură

- `Firmware/Src/game.c`: logică independentă de hardware, trei vieți, 18 inamici/val, proiectile și dificultate progresivă.
- `input.c`: ambele surse de comenzi, priorități, heartbeat și parser serial strict.
- `render.c`: grafică RGB565, font și sprite-uri proprii, fără bibliotecă grafică externă.
- `board.c`: ceas, pini, UART cu buffere și întreruperi, LCD prin LTDC/SPI5 și SDRAM prin FMC.
- `audio.c`: DAC canalul 2, TIM6, DMA1 Stream6; efecte pentru foc, impact, pierderea vieții, val nou și Game Over.
- `main.c`: buclă fără RTOS; actualizare logică la cel puțin 16 ms, afișare la aproximativ 30 fps și telemetrie la 5 Hz. Performanța exactă se măsoară pe placă.
- `PC/controller.py`: aplicație Tkinter, control tastatură, volum și starea primită de la placă.

Ceasul principal este 180 MHz, generat din HSI intern. UART1 este 115200 8N1 pe PA9/PA10. Două buffere RGB565 din SDRAM externă sunt schimbate la vertical blank, la `0xD0000000` și `0xD0040000`. Bufferul audio este în SRAM internă, accesibilă DMA. Nu se utilizează USB Host, SD, fișiere WAV sau FreeRTOS.

Protocolul și planul de testare pe placă sunt în [docs/PROTOCOL.md](docs/PROTOCOL.md) și [docs/VALIDATION.md](docs/VALIDATION.md).

## Licențe și surse

Codul propriu este sub licența MIT din `LICENSE`. Fișierele ST își păstrează licențele originale; vezi `THIRD_PARTY.md`. Îndrumarul utilizatorului este folosit drept referință hardware și nu este redistribuit în pachet.
