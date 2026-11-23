# Conexiuni CalculatorSTM32

Țintă: STM32F429I-DISCO / DISC1 cu STM32F429ZI, LCD ILI9341 240x320, touch rezistiv cu STMPE811 și SDRAM de 8 MB. Shield: Open429Z-D. Conexiunile și funcționarea perifericelor trebuie verificate pe placa reală; firmware-ul nu a fost încă probat pe hardware.

## Joystick

Pinii originali ai joystick-ului intră în conflict cu LCD-ul. Cu placa nealimentată, scoate cele cinci jumpere de pe headerul joystick-ului și conectează individual **partea care duce la contactul joystick-ului**, fără a păstra legătura către pinul PG original:

| Contact | Direcție din figura 2.7 a laboratorului | Pin original, de eliminat | Pin nou |
|---|---|---|---|
| JOY_B | Stânga | PG9 | PG2 |
| JOY_C | Dreapta | PG10 | PG3 |
| JOY_D | Sus | PG11 | PE2 |
| JOY_A | Jos | PG6 | PE3 |
| JOY_CTR | Apăsare centrală | PG12 | PE4 |

Comunul joystick-ului rămâne GND. Intrările noi au pull-up intern, active pe 0. Identifică partea jumperului după schemă sau prin continuitate cu alimentarea oprită; tabelul indică numele pinilor MCU, nu numere fizice ale headerelor. Nu apăsa joystick-ul cu legăturile originale spre PG6/PG10/PG11/PG12 păstrate în timp ce LCD-ul este activ.

Butonul USER rămâne pe PA0 și confirmă butonul selectat, la fel ca apăsarea centrală. RESET rămâne reset hardware. Astfel, joystick-ul singur permite utilizarea completă. Această mapare este aceeași ca în proiectul SpaceInvaders și poate fi adaptată în `board_config.h`.

## Touch integrat

Nu este necesar un display nou. Driverul folosește STMPE811 de pe placă:

| Funcție | Pin / configurație |
|---|---|
| I2C3 SCL | PA8 |
| I2C3 SDA | PC9 |
| Adresă STMPE811 | 0x41 pe 7 biți, 0x82 în API HAL |
| Citire | Polling, cu timeout limitat |

Pinii sunt rezervați pentru touch; nu conecta alte module care îi conduc. IRQ-ul touch nu este necesar în implementarea aceasta.

Transformarea implicită urmează calibrarea orientativă ST pentru reviziile mai vechi. Unele revizii D/E au axa Y inversată. Poți configura `MC_TOUCH_REVISION_D=1`, însă calibrarea în trei puncte este recomandată indiferent de revizie.

### Calibrare

1. Deschide SET folosind joystick-ul sau tasta V din aplicația PC.
2. Selectează CALIBRARE TOUCH.
3. Atinge centrul crucii, ridică degetul și repetă pentru cele trei poziții. Un stylus nemetalic cu vârf neascuțit poate îmbunătăți precizia.
4. După trei puncte valide revii la Setări. Datele sunt în RAM și se pierd la reset. Pentru o versiune finală, coeficienții pot fi fixați în configurație după măsurarea plăcii tale.
5. USER, centrul joystick-ului sau Escape anulează calibrarea. Dacă punctele sunt prea apropiate/coliniare, procesul se reia fără înlocuirea calibrării precedente.

În timpul calibrării, coordonatele brute sunt folosite direct, astfel că poți corecta și un ecran inversat sau foarte decalibrat. Nu calibra touch-ul fizic prin click pe reprezentarea PC a plăcii; atinge crucile de pe LCD. În simulator, click-urile pe ecran sunt touch virtual și permit testarea algoritmului.

## Comunicație serială

UART1 la 115200 baud, 8N1, fără flow control. Dacă revizia ta oferă VCP prin ST-LINK și legăturile către PA9/PA10 sunt prezente, folosește COM-ul respectiv. Nu presupune că vechiul ST-LINK/V2 al oricărui DISCO expune automat un port serial.

Alternativ, folosește un adaptor USB-UART cu **semnale de 3,3 V**:

| Adaptor | Placă |
|---|---|
| TXD | PA10 / RX |
| RXD | PA9 / TX |
| GND | GND |

Alimentează placa separat prin USB-ul ei normal. Nu conecta VCC/5 V al adaptorului pentru această legătură și nu folosi direct semnale RS-232 cu DB9. Evită conectarea în paralel a două emițătoare la PA10. Un singur program trebuie să deschidă COM-ul.

## Audio

Implementarea este pentru modulul ADC-DAC / Analog Test Board cu LM386M-1 și difuzor din figurile 6.7-6.8 ale îndrumarului. Identificarea modulului utilizatorului nu este încă confirmată.

| Modul analogic | Placă |
|---|---|
| POWER +5 V | 5 V |
| POWER GND | GND |
| INPUT DAC / H2, conform schemei modulului | PA5 / DAC_OUT2 |
| INPUT GND | GND |

Conectează intrarea amplificatorului, nu difuzorul sau ieșirea amplificatorului, la PA5. **Nu conecta un buzzer necunoscut sau difuzor direct la pinul DAC.**

Nu introduce întregul modul ADC-DAC în conectorul SPI1 ADC+DAC ca în exercițiul separat: un potențiometru ar ajunge pe PA6, utilizat acum pentru LCD. Folosește conexiuni individuale pentru alimentare, GND și DAC, lăsând ieșirile potențiometrelor neconectate.

Folosește mai întâi firmware-ul fără audio. După confirmarea modulului, încarcă varianta normală, verifică volumul 0 și crește treptat. Parametrul `MC_AUDIO_PEAK` limitează amplitudinea la 160 de trepte DAC în jurul nivelului central 2048. Limita trebuie validată pe modulul real, inclusiv pentru distorsiune.

## Indicatoare și resurse

LED verde: PC activ. LED roșu continuu: eroare aritmetică. La inițializare, grupurile de impulsuri roșii indică 1=ceas, 2=UART, 3=LCD, 4=SDRAM, 5=audio. Lipsa touch-ului nu este eroare fatală: apare NO TOUCH, iar driverul reîncearcă la aproximativ cinci secunde.

- [ST UM1670 - schemă funcțională, VCP și tabelul pinilor](https://www.st.com/resource/en/user_manual/dm00093903-discovery-kit-with-stm32f429zi-mcu-stmicroelectronics.pdf)
- [Schema ST MB1075 Rev B, inclusiv touch](https://www.st.com/resource/en/schematic_pack/mb1075-f429i-b01_schematic.pdf)
- [Driverul oficial ST STMPE811](https://github.com/STMicroelectronics/stm32-stmpe811)
- [Analog Test Board cu amplificator](https://www.waveshare.com/analog-test-board.htm)
