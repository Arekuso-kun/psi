# Protocol serial, versiunea 1

UART1, PA9 TX / PA10 RX, 115200 8N1. Mesaje ASCII terminate cu LF (`\n`). CR este ignorat. UART folosește întreruperi cu buffere RX/TX; logica jocului nu rulează în întreruperea UART.

## PC -> placă

| Mesaj | Efect |
|---|---|
| `K,05` | Starea completă a tastelor PC, mască hexazecimală 00..7F; exemplul înseamnă stânga + foc |
| `K,00` | Eliberarea tuturor tastelor PC |
| `VOL,30` | Setarea volumului la 30 din 100 |
| `STATE?` | Solicitarea imediată a stării |
| `PING` | Răspuns pong pentru verificarea legăturii |

| Bit | Valoare hex | Acțiune |
|---|---|---|
| 0 | 01 | Stânga |
| 1 | 02 | Dreapta |
| 2 | 04 | Foc |
| 3 | 08 | Volum + |
| 4 | 10 | Volum - |
| 5 | 20 | Pauză/reluare sau start din ecranul inițial |
| 6 | 40 | Start/restart din ecranul inițial sau Game Over |

`K` este o stare, nu un caracter repetat. Aplicația trimite la schimbare și cel puțin o dată la 100 ms. După 350 ms fără un `K` valid, starea PC este eliberată; butoanele locale continuă să funcționeze. PING și VOL nu țin apăsate tastele. Pauza și startul sunt declanșate numai la tranziția 0->1. Focul ținut apăsat nu repornește automat după Game Over: este necesară o nouă apăsare sau START.

Parserul respinge valorile în afara limitelor, textul suplimentar, măștile prea lungi și liniile prea lungi. Liniile peste 47 de caractere sunt abandonate până la următorul LF. După o eroare UART se resincronizează la LF. Nu se folosesc `eval`, comenzi de shell sau execuție de cod primit.

## Placă -> PC

La pornire:

```json
{"type":"hello","project":"SpaceInvaders","protocol":1}
```

Starea este trimisă la 200 ms, la `STATE?` și după `VOL`:

```json
{"type":"state","state":"playing","score":120,"best":120,"lives":3,"level":1,"volume":20,"pc":1,"audio":1,"uart_errors":0}
```

`state` poate fi `title`, `playing`, `paused`, `wave` sau `game_over`. `audio` arată dacă suportul DAC este activ în compilare; nu confirmă prezența fizică a difuzorului. `pc` indică un mesaj K valid în ultimele 350 ms. `uart_errors` numără erorile hardware sau depășirile bufferului RX.

Alte răspunsuri:

```json
{"type":"pong"}
{"type":"error","message":"bad_command"}
```

Pentru test manual poți trimite `PING`, `STATE?`, `VOL,0` și `VOL,20` din Tera Term. Mesajele K se trimit periodic din aplicație pentru menținerea comenzilor. Un terminal care trimite un singur `K,01` produce deplasare doar până la expirarea celor 350 ms.
