# Homework 3 - Fan Control and Status Panel

Acest proiect simulează un panou de control și status pentru un sistem de ventilație, folosind un microcontroler ESP32. Implementarea combină intrări analogice, semnale PWM, controlul LED-urilor (inclusiv RGB) și notificări sonore asincrone (fără funcția `delay()`).

## Hardware și Conexiuni

Toate componentele sunt conectate într-un singur circuit fix, rulând simultan. Potențiometrul este alimentat la **3.3V**.

| Componentă | Rol | Pin ESP32 |
| :--- | :--- | :--- |
| **Built-in LED** | System Heartbeat (indicator funcționare) | `GPIO 2` |
| **LED Extern** | Simulator viteză ventilator (Fan output) | `GPIO 18` |
| **Potențiometru** | Reglarea vitezei (Speed control) | `GPIO 34` |
| **LED RGB (R)** | Status viteză - Treimea superioară | `GPIO 27` |
| **LED RGB (G)** | Status viteză - Treimea mijlocie | `GPIO 26` |
| **LED RGB (B)** | Status viteză - Treimea inferioară | `GPIO 25` |
| **Buzzer pasiv** | Notificări sonore | `GPIO 23` |

## Implementare și Cerințe Rezolvate

1. **System Heartbeat:** 
   LED-ul integrat semnalează funcționarea sistemului printr-un tipar asincron (folosind `millis()`): clipește de 3 ori (interval de 250ms), urmat de o pauză de 2 secunde. Funcționează independent de celelalte componente.

2. **Fan Speed Control:** 
   ESP32 citește valoarea potențiometrului (0 - 4095) și o mapează la un interval PWM cu o rezoluție de 10-biți (0 - 1023) și o frecvență de 5000Hz. Frecvența și rezoluția ridicată ajută la obținerea unui output mult mai precis în raport cu citirea potențiometrului și elimină efectul de flickering al LED-ului. Astfel, luminozitatea LED-ului extern crește fin și proporțional cu valoarea citită.

3. **RGB Status Indicator:** 
   LED-ul RGB își schimbă culoarea în funcție de intervalul în care se află potențiometrul:
   * **LOW (Albastru):** Treimea inferioară a intervalului ADC.
   * **MEDIUM (Verde):** Treimea mijlocie a intervalului ADC.
   * **HIGH (Roșu):** Treimea superioară a intervalului ADC.

4. **Sound Notifications:** 
   Când potențiometrul intră în treimea superioară (zona Roșie), se declanșează o notificare sonoră pe buzzer. Sistemul reține starea pentru a nu repeta sunetul cât timp valoarea rămâne în acest interval (trigger doar la tranziția de stare).

5. **Non-blocking Implementation:** 
   Codul nu conține funcția `delay()`. Toate temporizările (clipirea LED-ului heartbeat, redarea notelor pe buzzer) sunt implementate asincron folosind `millis()`.

6. **Serial Debugging:** 
   La fiecare 500ms, sistemul afișează în Serial Monitor (Baud rate: 9600) următoarele date: valoarea raw ADC, valoarea PWM aplicată și nivelul de operare curent (LOW/MEDIUM/HIGH).

## Funcționalități Bonus Implementate

* **Melodie de notificare non-blocking:** În locul unui singur ton de alertă, la intrarea în zona HIGH este redată o scurtă melodie formată din 4 note (262Hz, 330Hz, 392Hz, 523Hz), rulată asincron.
* **Heartbeat Customizat:** LED-ul built-in nu execută un simplu blink, ci o secvență avansată de avertizare de funcționare (pachete de câte 3 blink-uri urmate de un cooldown logic).

## Media și Demonstrație

### Schemă Circuit și Montaj Fizic
*(Înlocuiți cu numele fișierelor voastre de imagine, de exemplu: schema.jpg și montaj.jpg)*
![Schema circuit](img/schema.jpg)
![Montaj asamblat](img/montaj.jpg)

### Video Demonstrație
Puteți viziona demonstrația completă a circuitului accesând link-ul de mai jos:
**https://youtu.be/TwHFxbFJ5SQ?is=crrc4eInEGL1LEV3**