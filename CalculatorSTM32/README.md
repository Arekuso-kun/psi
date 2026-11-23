# CalculatorSTM32

Calculator aritmetic pentru STM32F429I-DISCO / STM32F429I-DISC1 cu LCD 240x320. Poate fi folosit prin touch, joystick și tastatura PC-ului, în aceeași expresie, fără selectarea unui mod. Include clic la apăsare, volum și mute, calibrare touch în trei puncte și schimb de date UART în ambele sensuri.

**Proiectul este livrat ca director, fără arhivă.** Template-ul original și SpaceInvaders nu sunt modificate. Nu s-a programat automat nicio placă.

## Pornire rapidă

### Probă fără placă

Pe Windows x64, cu Python 3.10+ și Tkinter:

```powershell
python PC/controller.py --demo
```

Poți rula și `PC/Start_Demo.cmd`. DLL-ul simulatorului este inclus în `build/host`; nu trebuie să instalezi un compilator pentru a-l folosi. Click pe ecran simulează touch-ul, butoanele din stânga simulează joystick-ul, iar tastatura rămâne activă permanent. Sunt folosite aceleași fișiere C pentru calcule, comenzi, calibrare și desenare ca pe STM32. Simulatorul nu emulează perifericele fizice și nu redă sunetul.

### Placa reală

1. Citește [HARDWARE.md](docs/HARDWARE.md), în special recablarea joystick-ului și identificarea modulului audio.
2. Pentru prima probă, programează prin ST-LINK `build/CalculatorSTM32-no-audio/CalculatorSTM32-no-audio.hex`, cu modulul audio deconectat. Poți folosi STM32CubeProgrammer sau Keil. Pentru `.bin`, adresa de încărcare este `0x08000000`; HEX-ul include adresele.
3. Calculatorul funcționează autonom din touch sau joystick, cu alimentare normală. Dacă touch-ul nu se aliniază cu butoanele, deschide **SET > CALIBRARE TOUCH** folosind joystick-ul sau aplicația PC și atinge separat cele trei cruci. Calibrarea rămâne în RAM până la reset.
4. Pentru tastatura PC-ului:

   ```powershell
   python -m pip install -r PC/requirements.txt
   python PC/controller.py
   ```

   Selectează COM-ul și apasă **Conectează**. Aplicația afișează expresia și rezultatul primite de la placă; nu execută calculele în modul hardware. Imaginea PC în acest mod este o reprezentare a stării primite, nu un flux video de pe LCD.
5. După confirmarea modulului cu amplificator LM386 din laborator, conectează audio conform schemei și programează `build/CalculatorSTM32/CalculatorSTM32.hex`. Pornește la volum mic.

## Funcții și comenzi

- Operații `+`, `-`, `*`, `/`, cu prioritate: `2+3*4=14`.
- Numere zecimale și negative, schimbarea semnului ultimului operand, ștergere și golire.
- Rezultatul poate fi continuat cu un operator; o cifră introdusă după rezultat începe un calcul nou.
- Maximum 63 de caractere în expresie. Calcule în `double`, cu precizie finită; afișarea rezultatului are până la 10 cifre semnificative și poate folosi notație științifică. Pot apărea rotunjiri specifice reprezentării în virgulă mobilă.
- Sunt semnalate expresiile incomplete, împărțirea la zero, numerele în afara limitelor și expresiile prea lungi. Parantezele, procentele și memoria M+/M- nu sunt incluse.
- Backspace după un rezultat golește calculul; înainte de rezultat șterge ultimul caracter. Reapăsarea `=` recalculează expresia existentă, fără funcție de repetare a ultimei operații.

| Acțiune | Touch | Joystick / placă | Tastatură PC |
|---|---|---|---|
| Introducere | Butoane pe LCD | Direcții pentru selecție, apăsare centrală pentru activare | Cifre și operatori |
| Confirmă selecția | Apăsare directă | Centru sau USER | Space |
| Rezultat | `=` | Selectează `=` și confirmă | Enter sau `=` |
| Ștergere | DEL | Selectează DEL | Backspace / Delete |
| Golire | C | Selectează C | C sau Escape din calculator |
| Semn | +/- | Selectează +/- | F9 |
| Setări | SET | Sus de pe primul rând, apoi confirmare | V |
| Volum | -10 / +10 în Setări | Selectează și confirmă | + / - în Setări sau slider PC |
| Revenire | ÎNAPOI | Selectează ÎNAPOI | Escape |

O apăsare touch menținută sau trasă peste alte taste produce o singură tastă. Este necesară ridicarea degetului pentru următoarea apăsare. Confirmarea joystick/USER se declanșează o singură dată până la eliberare; navigarea ținută apăsată are autorepetare. Tastatura PC ignoră autorepetarea tastelor până la eliberare.

Nu se blochează celelalte surse după o comandă. La pierderea PC-ului, touch-ul și joystick-ul continuă imediat; indicatorul PC expiră după 2,5 secunde. Nu există comenzi de calcul menținute de PC după deconectare.

## Sunet

Fiecare tastă acceptată produce un clic de aproximativ 25 ms; o eroare poate produce un ton scurt distinct. Navigarea simplă nu produce sunet. Volumul 0..100 este o setare de amplitudine, nu un nivel în decibeli; 0 înseamnă mute, iar valoarea inițială este 20. Butonul MUTE revine la volumul anterior.

Ieșirea folosește PA5 / DAC_OUT2, TIM6 și DMA1 Stream6. Amplitudinea este limitată conservator, pentru intrarea modulului analogic cu LM386. Compatibilitatea unui buzzer separat trebuie verificată înainte de conectare. **Nu conecta un difuzor direct la PA5.**

## Structură

| Fișier | Responsabilitate |
|---|---|
| `Firmware/Src/calculator.c` | Parsare sigură, operații, editare și erori |
| `app.c` | Pagini, tastatură LCD, selecție, volum și comenzi comune |
| `input.c`, `joystick.c` | Coadă de evenimente, debouncing și autorepetare |
| `touch.c`, `calibration.c` | STMPE811 prin I2C3 și transformarea coordonatelor |
| `display.c`, `graphics.c` | Interfață RGB565 și font bitmap propriu |
| `audio.c` | Clic prin DAC / timer / DMA |
| `communication.c` | Mesaje ASCII de intrare și stare JSON de ieșire |
| `board.c` | Ceas, GPIO, UART cu întreruperi, LCD și SDRAM |
| `main.c` | Coordonarea perifericelor într-o buclă fără RTOS |
| `PC/controller.py` | Client serial și simulare opțională |

LCD-ul folosește LTDC și inițializare SPI5. Două buffere RGB565 în SDRAM externă sunt schimbate la vertical blank. Touch-ul STMPE811 este citit pe I2C3, PA8 SCL / PC9 SDA. UART1 folosește PA9 TX / PA10 RX la 115200 8N1. Ceasul principal este 180 MHz din HSI intern. Lipsa touch-ului nu oprește calculatorul; driverul este reîncercat periodic, iar joystick-ul și UART-ul rămân active.

## Compilare

### Keil

Deschide `MDK-ARM/CalculatorSTM32.uvprojx`. Selectează Arm Compiler 6 instalat în **Options for Target > Target**; metadatele inițiale indică 6.19. Instalează/selectează Device Family Pack pentru STM32F429ZI dacă este necesar și ST-LINK pentru programare/debug. Nu este necesar să regenerezi template-ul în CubeMX; proiectul nu include un `.ioc` nou.

Proiectul Keil este pregătit și referințele de fișiere sunt verificate. Compilarea în Keil nu a fost executată în această sesiune. Binarul furnizat a fost compilat cu GNU.

### GNU

Cu Arm GNU Toolchain disponibil:

```powershell
python tools/build.py
python tools/build.py --no-audio
```

Poți specifica `--toolchain "C:/cale/catre/arm-toolchain/bin"`. Scriptul caută și în `.tools` local, iar în workspace-ul actual poate reutiliza compilatorul portabil din `../SpaceInvaders/.tools`. Sursele proiectului sunt independente de SpaceInvaders; compilatorul este un instrument extern și nu a fost duplicat.

Suportul pentru formatarea rezultatelor în virgulă mobilă este inclus explicit în link-ul GNU. Sunt rezervate 16 KB pentru heap-ul folosit de biblioteca numerică și 8 KB pentru stack. Fișierele de configurare centrală sunt `Firmware/Inc/board_config.h` și `Firmware/STM32F429ZI_FLASH.ld`.

### Teste și verificare, fără arhivare

```powershell
python tools/build.py --host
python -m unittest discover -s tests -p "test_pc.py" -v
python PC/controller.py --demo --smoke-test
python tools/preview.py
python tools/validate.py
```

Pentru reconstruirea testelor native este necesar GCC host pentru Windows; simulatorul DLL inclus nu are această cerință la rulare. Sursele ST sunt deja incluse. `tools/import_platform.py` a fost utilizat numai la inițializare și refuză suprascrierea platformei existente.

Vezi [protocolul](docs/PROTOCOL.md), [conexiunile](docs/HARDWARE.md), [validarea](docs/VALIDATION.md) și [rezultatele build-ului](docs/BUILD_REPORT.md). Previzualizările din `docs/images` provin din rendererul C și nu sunt fotografii ale plăcii.
