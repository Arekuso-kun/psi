# Validare

## Verificări software

Comenzile reproductibile se găsesc în README. Testele C rulează pe aceleași module independente de hardware ca firmware-ul:

- Prioritatea operațiilor, asociativitatea, numere negative, zecimale și notație științifică generată pentru rezultate.
- Împărțire la zero, sintaxă incompletă, valori în afara limitelor și introducere peste 63 de caractere.
- Ștergere, schimbarea semnului, continuarea unui rezultat și începerea unui calcul nou.
- O expresie construită din touch, PC și confirmare joystick; apăsare touch ținută și glisare fără cifre suplimentare.
- Calibrare în trei puncte, respingerea punctelor degenerative și limitele coordonatelor.
- Debouncing și autorepetare pentru joystick, confirmare o singură dată.
- Mesaje invalide, recepție fragmentată, linie prea lungă, coadă plină și serializarea stării.
- Desenarea celor trei pagini fără scrieri în afara framebuffer-ului.

Testele Python verifică protocolul și integrarea clientului cu DLL-ul C: 500 de expresii generate determinist, amestecarea celor trei surse de comenzi și meniul de volum. Simulatorul execută logica și rendererul reale; nu este un emulator al microcontrolerului.

## Ce rămâne obligatoriu pe placa reală

1. Verifică placa și recablarea joystick-ului, cu alimentarea oprită. Nu păstra jumperele care conectează joystick-ul la liniile LCD.
2. Încarcă varianta fără audio. Confirmă LCD-ul și selectarea fiecărui buton cu joystick-ul și USER.
3. Calibrează touch-ul în trei puncte. Verifică toate colțurile și rândurile tastaturii, inclusiv DEL, semn, SET și butonul = lat.
4. Ține degetul pe o cifră și glisează: trebuie introdusă o singură cifră. Ridică și reapăsă pentru următoarea.
5. Conectează UART și verifică PING, STATE?, apoi aplicația PC. Ceea ce introduci din touch trebuie să apară pe PC și invers.
6. Introdu `2+` din touch, `3*` din tastatura PC și `4=` din joystick: rezultatul trebuie să fie 14. Nu trebuie selectat un mod.
7. Deconectează numai legătura de date, păstrând alimentarea plăcii. Calculul rămâne pe ecran și poate fi continuat local. PC ON dispare după aproximativ 2,5 secunde.
8. Testează `0.1+0.2`, `2*-3`, `1/0`, ștergere, C și continuarea unui rezultat. Precizia în virgulă mobilă și formatarea pe MCU trebuie confirmate pe placă, nu numai în libc de pe PC.
9. După identificarea modulului LM386, conectează-l conform HARDWARE.md și încarcă firmware-ul cu audio. Confirmă un clic pe tastă acceptată, liniște la navigare și la volum 0, revenirea din mute și lipsa distorsiunii la volumele utilizate.
10. Folosește calculatorul minimum 10 minute, alternând toate sursele și schimbând volumul. Urmărește erorile UART, stabilitatea touch-ului și reacția interfeței.

## Limitele verificării curente

Compilarea GNU și testele PC nu confirmă electric cablarea, orientarea touch-ului, timpii I2C, memoria externă reală sau amplitudinea acustică. Proiectul Keil este pregătit, dar nu a fost compilat cu Keil în această sesiune. Nicio placă nu a fost programată automat. Nu s-a creat o arhivă a proiectului.
