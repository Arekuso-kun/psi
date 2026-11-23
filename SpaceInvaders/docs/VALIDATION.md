# Verificare și prezentare

## Verificat automat pe calculator

- Compilarea și legarea firmware-ului pentru Cortex-M4F / STM32F429ZI, inclusiv versiunea fără audio.
- Teste native C pentru aceeași logică utilizată de microcontroler: coliziuni, scor, vieți, valuri, limitele navei, pauză, volume și rulare prelungită.
- Combinarea intrărilor: ultima direcție apăsată, revenirea la cealaltă sursă când se eliberează, expirarea PC și funcționarea în jurul depășirii contorului de timp.
- Validarea mesajelor PC și firmware; mesaje fragmentate, invalide și prea lungi.
- Încărcarea aplicației Tkinter, inclusiv cu simulatorul C și framebuffer-ul real.
- Previzualizarea graficii produse de rendererul C.

Aceste verificări nu validează electric hardware-ul sau temporizarea perifericelor pe placa reală. Simulatorul nu emulează STM32, USB/ST-LINK, SDRAM, UART sau DMA audio. Keil nu a fost disponibil; proiectul său trebuie compilat acolo.

## De verificat pe placă, în ordine

1. **Inspecție cablare:** placa nealimentată; cele cinci jumpere joystick originale scoase, contactele mutate conform HARDWARE.md. Fără legături ale modulului ADC pe PA6.
2. **Prima programare:** firmware fără audio, modul audio deconectat. Ecranul de start apare și nu pâlpâie vizibil. Nu apăsa joystick-ul până la confirmarea recablării.
3. **Comenzi locale:** toate direcțiile, foc, pauză și volum. Jocul pornește și funcționează fără aplicația PC.
4. **Schimb de date:** COM corect, 115200 8N1; placa răspunde la PING/STATE?. Aplicația arată scorul și viețile de pe LCD.
5. **Schimbarea sursei în aceeași partidă:** joystick -> tastatură -> joystick fără meniu. Ține stânga local și apasă dreapta pe PC; după eliberarea tastei, deplasarea locală se reia.
6. **Pierderea legăturii:** în timp ce o tastă PC este apăsată, închide aplicația sau deconectează numai adaptorul de date. Dacă placa este alimentată separat, comanda PC dispare în aproximativ 350 ms și joystick-ul continuă. Scoaterea singurului cablu de alimentare oprește, firesc, întreaga placă.
7. **Audio:** identifică modulul și verifică legăturile cu alimentarea oprită. Programează varianta audio. Verifică foc, impact, viață pierdută, val nou, Game Over, apoi mute și volume crescătoare. Confirmă că tonul nu se schimbă la reglarea volumului și că nu apar distorsiuni evidente.
8. **Volum în ambele sensuri:** modifică din joystick și observă PC-ul; apoi din slider/tastatură și observă LCD-ul. Volumul nu trece sub 0 sau peste 100.
9. **Joc complet:** trei vieți, Game Over, restart, pauză fără mișcarea proiectilelor și creșterea dificultății după un val. Recordul este păstrat până la reset.
10. **Rulare susținută:** minimum 10 minute cu grafica și sunetul active. Urmărește erorile UART, blocări, distorsiuni și stabilitatea LCD-ului.

## Diagnostic inițial

LED roșu cu grupuri de impulsuri separate de pauză:

| Impulsuri | Etapă |
|---|---|
| 1 | Ceas / OverDrive |
| 2 | UART |
| 3 | LCD / SPI / LTDC sau schimbarea framebuffer-ului |
| 4 | Test SDRAM |
| 5 | DAC / timer / DMA audio |

Un HardFault intră într-o buclă pentru inspectare cu debuggerul; nu este convertit în cod de impulsuri. La LCD nefuncțional, verifică întâi jumperele și eventualele module conectate pe pinii partajați.

## Ce poți demonstra profesorului

Joc autonom și preluarea comenzilor PC în aceeași partidă; UART în ambele sensuri; întreruperi și buffere; citirea GPIO și debouncing; timer, DAC și DMA pentru audio; volum comun; afișare LTDC cu două buffere în SDRAM; recuperare după deconectarea PC. Nu susține că aceste funcții au fost validate pe hardware până când parcurgi probele de mai sus.
