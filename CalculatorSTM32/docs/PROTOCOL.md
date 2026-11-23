# Protocol CalculatorSTM32, versiunea 1

UART1: PA9 TX, PA10 RX, 115200 8N1. Mesaje ASCII încheiate cu LF; CR este ignorat. Nu se execută cod sau expresii primite prin `eval`. Calculatorul primește taste individuale și își construiește intern expresia.

## PC -> placă

| Mesaj | Acțiune |
|---|---|
| `KEY,7` | Introduce cifra 7 |
| `KEY,+` | Operator +; în Setări crește volumul |
| `KEY,=` | Calculează rezultatul |
| `KEY,C` | Golește calculul |
| `KEY,DEL` | Șterge ultimul caracter |
| `KEY,SIGN` | Schimbă semnul |
| `KEY,SET` | Deschide / închide Setări |
| `KEY,BACK` | Revine din Setări sau calibrare; în calculator golește |
| `KEY,MUTE` | Comută sunetul, în Setări |
| `KEY,CAL` | Începe calibrarea, din Setări, dacă touch-ul este disponibil |
| `KEY,VOL+`, `KEY,VOL-` | Modifică volumul în Setări |
| `VOL,40` | Setează direct volumul 0..100, indiferent de pagină |
| `NAV,L`, `NAV,R`, `NAV,U`, `NAV,D` | Mută selecția |
| `OK` | Confirmă selecția; anulează pagina de calibrare |
| `STATE?` | Solicită imediat starea |
| `PING` | Verifică legătura; răspuns pong |

Alte taste valide: 0..9, `.`, `-`, `*`, `/`. `KEY,,` este acceptat ca separator zecimal, dar clientul normalizează la punct. Tastatura PC ignoră autorepetarea; un mesaj valid reprezintă un eveniment individual. Astfel, o deconectare nu poate lăsa o cifră sau o comandă apăsată.

Clientul trimite PING la o secundă. Indicatorul PC rămâne activ 2,5 secunde după ultimul mesaj valid. Expirarea nu șterge expresia și nu blochează controlul local.

## Placă -> PC

La pornire:

```json
{"type":"hello","project":"CalculatorSTM32","protocol":1}
```

Periodic, la aproximativ 200 ms, după schimbări și la STATE?:

```json
{"type":"state","page":"calculator","expression":"2+3*4","result":"14","error":0,"selected":18,"volume":20,"pc":1,"touch":1,"calibrated":0,"cal_step":0,"audio":1,"uart_errors":0,"dropped":0}
```

- `page`: `calculator`, `settings` sau `calibration`.
- `expression`: maximum 63 caractere; se construiește pe placă.
- `result`: ultimul rezultat formatat, până la 10 cifre semnificative. Poate utiliza notație științifică.
- `error`: 0=OK, 1=expresie incompletă/invalidă, 2=împărțire la zero, 3=limită numerică, 4=expresie prea lungă.
- `selected`: indexul butonului selectat; tastatura LCD are 19 taste plus SET.
- `touch`: driverul răspunde, fără a garanta calibrarea fizică.
- `calibrated`: s-a făcut calibrare în această sesiune; `cal_step` indică punctul 0..2 în timpul calibrării.
- `audio`: ieșirea DAC este inclusă în compilare; nu confirmă fizic prezența difuzorului.
- `uart_errors`: erori UART / depășiri RX; `dropped`: evenimente pierdute când coada este plină.

PING primește `{"type":"pong"}`. Un mesaj invalid primește `{"type":"error","message":"bad_command"}`. Dacă nu mai este loc în coadă, se trimite `queue_full` când bufferul TX permite.

## Limite și recuperare

Liniile de intrare au maximum 47 de caractere. După o linie prea lungă, un octet nevalid sau o eroare UART, recepția se resincronizează la LF. Coada are 31 de evenimente utile. TX și RX folosesc buffere cu întreruperi; calculele și accesul la LCD nu se execută în întreruperea UART. Clientul PC limitează liniile JSON la 1024 de octeți și verifică tipurile și domeniile câmpurilor.
