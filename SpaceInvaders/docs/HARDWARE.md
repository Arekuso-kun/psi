# Conexiuni hardware

Firmware-ul țintește STM32F429I-DISCO / STM32F429I-DISC1 cu STM32F429ZI, LCD ILI9341 240x320 și SDRAM de 8 MB. Shield: Open429Z-D. Verifică inscripțiile și schema reviziei tale înainte de cablare. Proiectul nu a fost încă probat pe placa fizică.

## 1. Joystick și LCD

Configurația din laborator conectează joystick-ul la PG6, PG9, PG10, PG11 și PG12. PG6, PG10, PG11 și PG12 sunt linii RGB ale LCD-ului. **Nu apăsa joystick-ul cu aceste legături păstrate în timp ce rulează LCD-ul.**

Cu alimentarea deconectată, scoate toate cele cinci jumpere ale joystick-ului de pe headerul P6 al shield-ului. Folosește partea headerului conectată la contactele joystick-ului drept sursă, nu partea care duce la pinii PG originali. Dacă partea nu este evidentă, identific-o prin schemă/continuitate cu placa nealimentată.

| Contact joystick | Poziție din figura 2.7 | Legătură originală, de eliminat | Legătură nouă |
|---|---|---|---|
| JOY_B | Stânga | PG9 | PG2 |
| JOY_C | Dreapta | PG10 | PG3 |
| JOY_D | Sus | PG11 | PE2 |
| JOY_A | Jos | PG6 | PE3 |
| JOY_CTR | Apăsare centrală | PG12 | PE4 |

Pinul comun al joystick-ului rămâne la GND. Intrările noi folosesc pull-up intern și sunt active pe 0. Identifică pinii PG2/PG3/PE2/PE3/PE4 după schema conectorilor P1/P2 ai plăcii; tabelul de mai sus folosește numele microcontrolerului, nu numere fizice ale headerelor. Nu lega între ele partea joystick și pinul PG original după mutare.

Butonul USER al plăcii rămâne pe PA0, activ pe 1. RESET rămâne reset hardware. LED-ul verde PG13 arată heartbeat PC activ, iar LED-ul roșu PG14 indică invulnerabilitate temporară în joc sau un cod de eroare la inițializare.

Maparea poate fi schimbată în `Firmware/Inc/board_config.h`. Pinii aleși nu sunt utilizați de LCD, SDRAM, UART1 sau ieșirea audio din acest proiect; alte module conectate la shield pot necesita verificări suplimentare.

## 2. Comunicație PC - placă

UART1: **115200 baud, 8 biți, fără paritate, 1 stop, fără flow control**.

### Dacă ST-LINK oferă port COM

Verifică în Device Manager dacă apare un port serial și dacă USART1 PA9/PA10 este conectat la ST-LINK pe revizia ta. STM32F429I-DISC1 cu ST-LINK/V2-B și punțile configurate corespunzător suportă VCP; pe vechiul DISCO nu trebuie presupus că simplul cablu de programare oferă și UART.

### Alternativ: adaptor USB-UART, semnale de 3,3 V

| Adaptor USB-UART | Placă |
|---|---|
| TXD | PA10 / USART1_RX |
| RXD | PA9 / USART1_TX |
| GND | GND |

Alimentează placa prin conexiunea ei normală USB; pentru această legătură UART nu conecta ieșirea 5 V/VCC a adaptorului. Este nevoie de niveluri TTL 3,3 V, nu RS-232 cu DB9 și nu semnale TTL de 5 V. Folosește o singură sursă care conduce linia RX a plăcii; nu lega în paralel două emițătoare UART.

## 3. Audio - numai după identificarea modulului

Implementarea este pentru **modulul Analog Test Board / ADC-DAC cu amplificator LM386M-1 și difuzor**, prezentat în figurile 6.7-6.8 ale îndrumarului, pag. tipărită 107 (pagina PDF 109). Nu presupune compatibilitate cu un buzzer activ separat.

Pe acest modul, traseul este: PA5 / DAC_OUT2 -> intrare DAC a modulului -> condensator și amplificator LM386 -> difuzor. Folosește intrarea analogică a modulului, nu ieșirea amplificatorului.

| Modul analogic | Placă |
|---|---|
| POWER +5 V | 5 V |
| POWER GND | GND |
| INPUT DAC (H2, conform schemei modulului) | PA5 / DAC_OUT2 |
| INPUT GND | GND |

**Nu conecta un difuzor sau buzzer necunoscut direct la PA5.** PA5 este ieșire de semnal, nu amplificator de putere. Confirmă polaritatea headerului de alimentare de pe modulul real.

Nu introduce automat întregul modul în conectorul SPI1 ADC+DAC ca în exercițiul separat de laborator: unul dintre potențiometre ar ajunge pe PA6, care în acest proiect este ieșire RGB pentru LCD. Pentru prima implementare folosește numai legăturile individuale pentru alimentare, GND și DAC; lasă ieșirile potențiometrelor neconectate.

Volumul este controlat din software. La pornire: 20/100; amplitudinea maximă este limitată în `SI_AUDIO_PEAK` la 160 trepte DAC în jurul nivelului central 2048. Limita este conservatoare și trebuie verificată după identificarea modulului pentru a evita distorsiunea. Nu modifica frecvența tonului pentru a modifica volumul.

Pentru prima probă folosește HEX-ul `SpaceInvaders-no-audio`, fără modulul audio. După confirmarea modulului, utilizează firmware-ul normal, pornește de la volum mic și verifică mute și fiecare efect.

## Resurse

- [Manual ST UM1670, tabelul 7 și configurația VCP](https://www.st.com/resource/en/user_manual/dm00093903-discovery-kit-with-stm32f429zi-mcu-stmicroelectronics.pdf)
- [Modulul Analog Test Board](https://www.waveshare.com/analog-test-board.htm)
- [ST AN3126 - generare audio cu DAC, timer și DMA](https://www.st.com/resource/en/application_note/an3126-how-to-use-dac-for-audio-and-waveform-generation-on-stm32-mcus-and-mpus-stmicroelectronics.pdf)
- Îndrumarul local: `../Microcontrolere.pdf` raportat la directorul principal SpaceInvaders, figurile 2.7 și 6.8.
