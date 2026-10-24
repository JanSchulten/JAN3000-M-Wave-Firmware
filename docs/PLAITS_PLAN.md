# Plaits-Engines: Plan und Machbarkeit (Phase 0)

Ziel: Fünf Oszillator-Modelle aus Mutable Instruments' Plaits (das Modul, auf dem Behringers Grind und Brains
aufbauen) als eigene Engines auf den FM-1 bringen, jeweils wie am Grind als Oszillator → Filter → ADSR. Dafür gehen
fünf bestehende Engines. Dieses Dokument hält die Entscheidungen fest und was Phase 0 gemessen hat.

## 1. Entscheidungen

| Raus | Rein (Plaits) | Gespeicherte Sounds der alten Engine laden als |
| --- | --- | --- |
| PHASE | **WAVESHAPE** (Waveshaping) | WAVESHAPE |
| VOICE | **WAVETABLE** | WAVETABLE |
| TRIO | **HARMONIC** (Additive) | ANALOG |
| DBOX | **PARTICLE** | DRUM (Kit-Lanes und Pattern bleiben) |
| SLICE | **SWARM** | SWARM |

- Fünf eigene Engines, keine Sammel-Engine mit MODEL-Parameter.
- Die neuen Engines werden an `ENGINES[]` angehängt (17 … 21); die alten Nummern bleiben reserviert und werden beim
  Laden umgewandelt, wie DIGITAL → FM6 (`fm4_convert.c`). Im PRESETS-Menü rücken die neuen an die Stelle der alten.
- Polyphonie: so viele Stimmen, wie die CPU schafft, pro Engine gemessen.
- Werks-Presets bringen keine Effekte mehr mit (umgesetzt, Abschnitt 4).
- PARTICLE: ein Diffusor pro Part, die Verzögerungen auf ~6 000 Samples gekürzt (12 KB, im Engine-Speicher des Parts).
- WAVETABLE: 16-Bit-Tabellen, wenn der Flash nach dem Aufräumen reicht, sonst 8 Bit.

## 2. Was Phase 0 gemessen hat

### Lizenz

Alle fünf Engines (`plaits/dsp/engine/{waveshaping,wavetable,additive,particle,swarm}_engine.cc`) und die stmlib
stehen unter MIT (Emilie Gillet). Das verträgt sich mit GPL-3.0; die MIT-Notiz muss mit in jede portierte Datei. Der
Name „Mutable Instruments“ darf nicht für das Ergebnis werben. Übrigens liegen auch die neueren Plaits-1.2-Modelle
(`plaits/dsp/engine2/`) im offenen Repository.

### Der Chip hat eine FPU

JieLis eigenes SDK baut für den WL82 (AC791N) mit `-mcpu=r3 -mfprev1` (`apps/demo/*/board/wl82/Makefile`): der Chip
hat eine Gleitkomma-Einheit. Felucca baut bisher ohne und rechnet ganzzahlig. Die Toolchain kann die FPU **pro
Funktion** einschalten: `__attribute__((target("fprev1")))`. Nur die Plaits-Funktionen bekommen sie; der Rest der
Firmware bleibt Befehl für Befehl wie er ist. Gemessen am Prototyp:

- mit dem Attribut: keine Soft-Float-Aufrufe mehr, Addition, Multiplikation, Division und Umwandlung sind
  einzelne Befehle (`r0 = r0 * r1 (f)`, `ftoi`, `itof`);
- ohne: 81 Aufrufe von Bibliotheksfunktionen (`__mulsf3` …) pro Sample-Schleife: unbrauchbar;
- Falle: jede Hilfsfunktion, die Float rechnet, braucht das Attribut auch, sonst wird sie nicht eingebettet und
  läuft als Soft-Float.

**Offen und nur am Gerät zu klären:** ob Feluccas eigener Start (`crt0.S`, `main.c`) die FPU schon nutzbar macht
(das Startup des SDK liegt nur als Bibliothek vor) und wie viele Takte ein FPU-Befehl kostet. Vorschlag: eine kleine
Probe in MENU > SYSTEM, die erst auf Tastendruck ein paar FPU-Befehle ausführt und die Zeit misst. Läuft sie nicht,
startet das Gerät nur neu (kein Boot-Problem, denn sie läuft nie beim Start).

### Waveshaping-Prototyp

Die Waveshaping-Engine nach C portiert (Float, FPU-Attribut) und Sample für Sample gegen das Original-C++ geprüft:

| Variante | Abweichung vom Original (240 000 Samples) |
| --- | --- |
| exakt | 0 (bitgleich) |
| schnell (Kehrwerte der Pulsbreite interpoliert statt dividiert) | −50 dB, Spitzen bis 0,49 während MORPH sich bewegt |

### Rechenaufwand

Statisches Kostenmaß wie `tests/target_budget.py` (Befehle in Schleifen, ×4 je Verschachtelung, Division ×9), am
pi32v2-Assembly des echten Builds gezählt. Kein Taktzähler, aber dasselbe Maß für alle:

| Engine | Kosten pro Stimme | | Bestehende Engine | Kosten |
| --- | --- | --- | --- | --- |
| WAVESHAPE (2 Oszillatoren + Formung) | ~400 | | ANALOG | 559 |
| WAVETABLE | ~380 | | FM6 (+ Operatoren) | 244 + |
| HARMONIC (Additive) | ~280 | | VOICE | 254 |
| PARTICLE (+ Diffusor) | ~360 | | TRIO | 244 + 135 |
| SWARM (8 Körner) | ~370 | | NOISE | 236 |

Die Plaits-Engines liegen damit im Bereich von ANALOG, nicht darüber. Was eine Stimme wirklich an Takten kostet,
hängt an den FPU-Latenzen (siehe oben) und wird am Gerät gemessen; danach wird die Stimmenzahl pro Engine festgelegt.
Plaits rechnet zusätzlich einen AUX-Ausgang; den brauchen wir nicht, das spart einen Teil.

### Speicher

| | Bedarf |
| --- | --- |
| Code der fünf Engines (pi32v2, `-Os`) | je 1,5 … 2,1 KB, zusammen ~9 KB, dazu die Felucca-Hülle (Parameter, Presets) |
| Tabellen | Sinus 2,5 KB, Falter 4,1 KB (als int16 2 KB), Waveshaper 2,6 KB |
| Wavetables | 25 344 Werte: **50,7 KB** in 16 Bit, 25,3 KB in 8 Bit |
| RAM: PARTICLE-Diffusor | 8 093 Samples Verzögerung → 16 KB je Instanz |
| RAM: der Rest | klein (SWARM ~0,5 KB je Stimme, im Engine-Speicher des Parts) |

Flash heute: Image 557 324 B von 581 564 B, **24 240 B frei** (die 72 KB in `hardware.md` waren veraltet).
Was das Entfernen freimacht:

| | Flash | RAM |
| --- | --- | --- |
| SLICE samt BREAK-Sample (22 050 B) und UI, gemessen (`FELUCCA_SLICE=0`) | 31 532 B | 4 960 B |
| PHASE, VOICE, TRIO, DBOX: Code, geschätzt aus `nm` (ohne Presets und Texte) | ~11 500 B | ~0,5 KB |
| **frei danach, zusammen mit den 24 240 B von heute** | **~67 KB** | |

Bedarf der fünf neuen Engines: Code und Hülle ~15 KB, Tabellen ~9 KB, dazu die Wavetables. Mit **8-Bit-Wavetables
(~50 KB zusammen) passt es mit ~17 KB Reserve**; mit 16 Bit (~75 KB) nur, wenn das Entfernen mehr bringt als
geschätzt (wird in Schritt 4 gemessen).

## 3. Offene Punkte

1. **WAVESHAPE exakt oder schnell:** nach einem Hörvergleich.
2. **Was die FPU auf dem Gerät kostet:** die FPU-Probe (Abschnitt 6).

## 4. Umgesetzt: Werks-Presets ohne Effekte

Ein Werks-Preset bringt nur noch den Sound: die Sends der Spur (DIST, CHORUS, DELAY, REVERB) und ihr INSERT bleiben,
wie sie sind, beim PRESETS-Knopf, im Editor, bei Program Change, beim Wechsel der Engine und bei den zufälligen
Sounds eines neuen Projekts. Ein User-Preset bringt die Effekte mit, mit denen es gespeichert wurde. Nur die
Einschalt-Sounds laden ihre Sends wie bisher, damit das Einschalt-Projekt und Projekte im alten Format klingen wie
immer (`ui.c` `preset_fx`, `preset_load_to`; Test: `tests/preset_fx_test.c`).

## 5. Reihenfolge

1. ~~Werks-Presets ohne Effekte~~ (erledigt)
2. ~~Phase 0: Machbarkeit~~ (dieses Dokument)
3. FPU-Probe auf dem Gerät (gebaut, Abschnitt 6; die Messung am Gerät steht aus)
4. Alte Engines umwandeln und entfernen (SLICE, PHASE, VOICE, TRIO, DBOX), Tests für das Laden alter Projekte
5. Neue Engines: WAVESHAPE → HARMONIC → PARTICLE → WAVETABLE → SWARM
6. Presets, UI, Golden-Tests, Emulator; erst dann aufs Gerät, nur mit Okay

## 6. Die FPU-Probe (MENU > SYSTEM > INFO)

`firmware/src/fpu_probe.c` misst am Gerät, was der Host nicht kann. Unter den drei Zeilen von INFO steht ein zweiter
Block: **FPU**, **F I DIV NS**, **WS VOICE**. OCT+ startet die Probe, jeder Druck einmal:

- **FPU:** `OK`, wenn ein paar Ergebnisse bitgenau stimmen (Multiplizieren-Addieren, Division, Float ↔ Int) und
  16 Blöcke des Waveshapers denselben Hash ergeben wie auf dem Host (`FPB_WS_HASH`); `WS DIFFERS`, wenn nur der
  Waveshaper abweicht; `WRONG`, wenn schon die einfachen Ergebnisse falsch sind.
- **F I DIV NS:** Nanosekunden für ein Float-Multiplizieren-Addieren, ein Integer-Multiplizieren-Addieren und eine
  Float-Division (je eine abhängige Kette, die kürzeste von 16 Messungen, TIMER4 mit 24 MHz).
- **WS VOICE:** eine Stimme des Waveshapers (`plaits_ws.c`, bitgleich mit Plaits), ein 32-Sample-Block, in Prozent
  der Zeit, die das Audio dafür hat (32 Samples bei 44,1 kHz). Daraus folgt die Stimmenzahl.

Sicherheit: Die Probe läuft nur auf Tastendruck in der Hauptschleife, nie beim Start. In den ersten 30 s nach dem
Einschalten verweigert sie („FPU TEST AFTER 30 S“), solange der Boot-Schutz den Start noch mitzählt. Ein Absturz
kann also nie als fehlgeschlagener Start zählen. Stürzt sie ab, weil die FPU aus ist, zeigt das Gerät den
Crash-Bildschirm und startet nach 4 s neu. Vorher das Projekt speichern: Nach einem Absturz lädt RESTORE LAST nicht
automatisch.

Kosten: 12,2 KB Flash (vor allem die Plaits-Tabellen, die WAVESHAPE ohnehin braucht), 336 B RAM. Das Image hat
keine Soft-Float-Hilfsfunktion (`__mulsf3` …) gelinkt.
