# Rezultatul verificărilor software

Data: 16 noiembrie 2026. Proiect livrat în directorul CalculatorSTM32, fără arhivare și fără programarea unei plăci.

## Firmware

Compilat cu Arm GNU Toolchain 13.3.Rel1, GCC 13.3.1, Cortex-M4F, Thumb, hard-float, optimizare pentru dimensiune. Ambele variante au compilare și link reușite, fără avertismente în build-ul final.

| Variantă | text | data | bss |
|---|---:|---:|---:|
| CalculatorSTM32, audio activ | 52712 | 548 | 28216 |
| CalculatorSTM32-no-audio | 50792 | 548 | 27496 |

Valorile sunt în octeți, conform arm-none-eabi-size. `bss` include rezerva de heap de 16 KB și stack de 8 KB. Cele două framebuffere RGB565 folosesc în total 300 KB din SDRAM externă, separat de SRAM internă raportată.

Au fost generate ELF, HEX și BIN în directoarele build respective. HEX-urile au trecut verificarea checksum-urilor, a domeniului Flash și a vectorilor de boot. Datele ocupă 53260 de octeți în HEX-ul cu audio și 51340 în cel fără audio.

## Teste pe PC

- Testele native C au trecut: evaluare, prioritate, zecimale, numere negative, erori, editare, comenzi combinate, touch unic, calibrare, joystick, protocol, coadă și limitele framebuffer-ului.
- Cele șase teste Python au trecut, inclusiv 500 de expresii generate determinist și un calcul introdus folosind touch virtual, tastatură și joystick virtual.
- Aplicația Tkinter a trecut proba de pornire atât în modul demo, cât și fără un port serial conectat. Dimensiunea cerută de interfața demo a fost verificată pentru a încăpea în fereastra inițială.
- Cele trei pagini LCD au fost exportate din rendererul C pentru inspecție vizuală, în docs/images.
- 47 de fișiere vendor descărcate au hash-uri SHA256 conforme cu vendor-lock.json.
- Cele 45 de referințe de surse din proiectul Keil indică fișiere existente.
- Scripturile Python au trecut verificarea de sintaxă.
- Validarea confirmă lipsa unei arhive CalculatorSTM32.zip și a arhivelor ZIP în directorul proiectului.

## Limite

Nu a fost disponibilă o placă fizică pentru teste. Touch-ul, cablarea, sunetul, UART-ul real și funcționarea LCD/SDRAM trebuie probate conform VALIDATION.md. Keil nu a fost executat; proiectul său este pregătit, iar binarele incluse provin din GNU. Simulatorul nu validează electric perifericele și nu redă sunet.

Rezultatul automat al verificării fișierelor este în build/validation.json.
