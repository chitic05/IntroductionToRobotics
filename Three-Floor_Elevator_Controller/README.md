# Tema #6 - Three-Floor Elevator Controller

## Descrierea Proiectului

Acest proiect implementează un sistem de control pentru un **lift cu trei etaje** folosind microcontrolerul ESP32 și framework-ul nativ ESP-IDF. Sistemul răspunde la comenzile fizice de la butoanele de etaj, simulează deplasarea între etaje și operarea ușilor, oferind totodată feedback vizual (afișaj cu 7 segmente și LED-uri de stare) și auditiv (buzzer pasiv controlat prin PWM).

Arhitectura este bazată pe evenimente și combină o mașină de stări finite (FSM), timere hardware de înaltă rezoluție (`esp_timer`), sincronizare lock-free între nuclee prin variabile atomice (`std::atomic<bool>`), precum și controlul afișajului printr-un registru de deplasare 74HC595.

---

## Componente Hardware și Conexiuni

Circuitul este realizat pe un montaj fix, respectând nivelul logic de 3.3V al ESP32:

| Componentă | Rol în Sistem | Pin ESP32 | Mod / Configurare |
| :--- | :--- | :--- | :--- |
| **Buton Etaj 1** | Cerere deplasare la Etajul 1 | `GPIO 34` | Input-only (necesită pull-up extern 10kΩ) |
| **Buton Etaj 2** | Cerere deplasare la Etajul 2 | `GPIO 39` | Input-only (necesită pull-up extern 10kΩ) |
| **Buton Etaj 3** | Cerere deplasare la Etajul 3 | `GPIO 36` | Input-only (necesită pull-up extern 10kΩ) |
| **LED Cerere Etaj 1** | Indicator cerere activă Etaj 1 | `GPIO 21` | Digital Output |
| **LED Cerere Etaj 2** | Indicator cerere activă Etaj 2 | `GPIO 22` | Digital Output |
| **LED Cerere Etaj 3** | Indicator cerere activă Etaj 3 | `GPIO 23` | Digital Output |
| **LED Mișcare Lift** | Indicator deplasare (clipește în mișcare) | `GPIO 17` | Digital Output |
| **Buzzer Pasiv** | Notificări audio uși / sosire | `GPIO 16` | LEDC PWM (`LEDC_TIMER_0`, `CHANNEL_0`) |
| **74HC595 Data (DS)** | Date seriale pentru afișaj 7 segmente | `GPIO 19` | Digital Output |
| **74HC595 Shift Clock (SHCP)** | Ceas deplasare biți | `GPIO 18` | Digital Output |
| **74HC595 Latch (STCP)** | Latch actualizare afișaj | `GPIO 5` | Digital Output |

*Notă hardware: Pentru segmentele afișajului cu 7 segmente s-au utilizat rezistoare de limitare a curentului de **1000 Ω**, iar pentru LED-urile de stare (indicatoarele de etaj și LED-ul de mișcare) s-au folosit rezistoare de **220 Ω**. Pinii `GPIO 34`, `GPIO 36` și `GPIO 39` sunt pini exclusiv de intrare pe ESP32 și nu dispun de rezistoare interne de pull-up, motiv pentru care sunt conectați cu pull-up extern la 3.3V.*

---

## Cerințe Implementate

### 1. Stare Inițială și Afișarea Etajului Curent
La pornire, liftul este inițializat la primul etaj (indexat intern `0`, corespunzător Etajului 1) în starea `IDLE` (cu ușile închise), iar afișajul cu 7 segmente indică etajul curent prin intermediul clasei `ShiftRegister`. Pe parcursul deplasării, afișajul se actualizează progresiv la trecerea prin fiecare etaj intermediar.

### 2. Citirea Butoanelor și Debouncing Software
Citirea celor trei butoane se realizează asincron printr-un timer periodic (`inputTimerHandle`) configurat la un interval de **3 ms**. Fiecare buton utilizează un registru de deplasare pe 8 biți (`history`) pentru filtrarea zgomotului mecanic (debouncing) și detecția frontului (`0x00` la apăsare, `0xFF` la eliberare), prevenind înregistrarea multiplă la o singură apăsare sau la menținerea butonului apăsat.

### 3. Mașina de Stări a Liftului
Logica principală este structurată într-o mașină de stări finite cu 4 stări:
- **`IDLE`**: Liftul staționează cu ușile închise și verifică coada de cereri prin `findNextFloor()`. Dacă există o cerere la alt etaj, activează indicatorul de mișcare și trece direct în `MOVING`; dacă se apasă butonul etajului curent, trece în `OPENING_DOORS`.
- **`MOVING`**: Deplasează liftul etaj cu etaj (câte **1 secundă** per etaj), actualizând afișajul la fiecare nivel și verificând dacă etajul curent se află în coada de cereri pentru a opri în `OPENING_DOORS`.
- **`OPENING_DOORS`**: Elimină cererea din coadă, stinge LED-ul etajului deservit și simulează deschiderea ușilor timp de **2 secunde** cu semnal sonor de sosire, trecând apoi în `CLOSING_DOORS`.
- **`CLOSING_DOORS`**: Simulează închiderea ușilor timp de **2 secunde** cu semnal sonor dedicat și revine în `IDLE` (de unde preia imediat următoarea cerere din coadă, dacă există).

### 4. Indicatoare LED pentru Cereri și Mișcare
- **LED-urile de etaj (`GPIO 21, 22, 23`)**: Se aprind imediat la înregistrarea unei cereri și rămân aprinse până când liftul ajunge la etajul respectiv și deschide ușile.
- **LED-ul de mișcare (`GPIO 17`)**: Este controlat independent printr-un timer periodic dedicat de **300 ms** (`blinkTimerArgs`). Clipește exclusiv cât timp flag-ul atomic `isMoving` este activ și rămâne stins când liftul este oprit.

### 5. Notificări Audio prin LEDC PWM
Buzzer-ul pasiv este pilotat de perifericul hardware LEDC (rezoluție pe 10 biți) pentru a genera două tonuri distincte:
- **Închidere uși**: Notă gravă (`Sol4` — **392 Hz**, duty cycle `128`) pentru un sunet discret de avertizare.
- **Deschidere uși / Sosire**: Notă înaltă (`Do6` — **1047 Hz**, duty cycle `512`) pentru un semnal clar de confirmare.

### 6. Coada de Cereri și Planificare Inteligentă (Bonus)
Cererile primite în timp ce liftul este ocupat sunt memorate într-un vector de stări atomice (`inQueue`), evitând dublarea opririlor pentru același etaj. Funcția `findNextFloor()` implementează un algoritm de planificare de tip **SCAN (algoritmul liftului)**: liftul își continuă deplasarea în direcția curentă (`direction = 1` pentru urcare, `-1` pentru coborâre) pentru a deservi toate cererile aflate în drum înainte de a inversa sensul.

### 7. Sincronizare Lock-Free și Operare Non-Blocantă
Citirea butoanelor și controlul clipirii LED-ului rulează în task-ul `esp_timer` independent de întârzierile din bucla principală, permițând înregistrarea comenzilor în orice stare a liftului. Accesul concurent la coada de cereri (`inQueue`) și la starea de mișcare (`isMoving`, `ledState`) între task-ul principal și timerele `esp_timer` este sincronizat **lock-free** prin variabile atomice (`std::atomic<bool>`), garantând consistența memoriei între nucleele ESP32 fără a folosi mutex-uri sau blocaje.

### 8. Diagnosticare prin ESP-IDF Logging
Evenimentele importante (inițializarea, recepționarea comenzilor, direcția de deplasare, trecerea prin etaje și tranzițiile de stare) sunt raportate în Serial Monitor prin `ESP_LOGI` sub tag-ul `"ELEVATOR"`.

---

## Funcționalități Bonus Implementate

- **Registru de deplasare 74HC595**: Utilizat pentru comanda afișajului cu 7 segmente prin doar 3 pini GPIO (`DS`, `SHCP`, `STCP`), încapsulat în clasa `ShiftRegister`.
- **Planificare inteligentă a etajelor (SCAN)**: Deservirea cererilor pe traseu în funcție de direcția curentă de deplasare, înainte de schimbarea sensului.
- **Efecte sonore diferențiate**: Frecvențe și niveluri de volum (duty cycle) distincte pentru închiderea și deschiderea ușilor generate hardware prin LEDC.

---

## Materiale Suplimentare

### Schemă Circuit și Montaj
![Schema circuit](./img/schema.jpeg)
![Montaj asamblat](./img/montaj.jpeg)

### Demonstrație Video
În videoclipul de mai jos este prezentat circuitul asamblat și funcționarea completă a liftului (pornire la etajul 1, cereri multiple în timpul deplasării, semnale sonore și actualizarea afișajului):

[**Urmăriți Demonstrația Video pe YouTube**](https://youtu.be/iwA4T9aKTjE)