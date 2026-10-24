# Checkliste: die Plaits-Engines auf dem FM-1

Stand: Firmware mit allen fünf Plaits-Engines (WSHAPE, HRMNC, PRTCL, WTABLE, SWARM). Alles hier ist auf dem Host
getestet (Tests, Golden-Renders, bitgleich gegen Plaits' Originalcode), aber **noch nicht auf dem Gerät gelaufen**.
Was nur das Gerät zeigen kann, steht unten zum Abhaken. Details: [PLAITS_PLAN.md](PLAITS_PLAN.md).

## 1. Installieren

- [ ] `dist/M-Vave-OP1-Installer.zip` wie immer installieren.
- [ ] Startet normal (kein Hänger, kein Sprung in den Updater).
- [ ] MENU > SYSTEM > INFO zeigt die neue Version.

## 2. Die Engines in der Liste

- [ ] PRESETS: nach FM6 kommen **WSHAPE, HRMNC, PRTCL, WTABLE, SWARM**, danach LOFI … VDRUM wie bisher.
- [ ] EDIT gehalten + weiße Taste: 16 Engines auf F3 … G5 (alle weißen Tasten belegt), INIT auf F#5 wie bisher.
- [ ] Namen und Seiten-Titel nirgends abgeschnitten (PRESETS, EDIT, HOME).

## 3. Anhören, pro Engine

Für jede Engine: alle 8 Presets durchspielen, tief und hoch; HOME KNOB 1..4 = HARM TIMB MORPH CUT; AUX von 0 bis 127.

| Engine | Worauf hören | ok? |
| --- | --- | --- |
| WSHAPE | HARM ändert die Kurve, TIMB faltet (heller), MORPH macht den Dreieck schief | [ ] |
| HRMNC | TIMB schiebt den Spitzen-Oberton hoch, MORPH 127 = fast Sinus, AUX = Orgel. Helle Presets werden ganz oben leiser (gewollt, wie Plaits) | [ ] |
| PRTCL | MORPH unter 64: Nachhall nach dem Loslassen, über 64: tonale Pings; jeder Ton beginnt mit einem Klick | [ ] |
| WTABLE | TIMB / MORPH / HARM laufen durch die Wellen; HARM über der Mitte: in Stufen statt fließend | [ ] |
| SWARM | jeder Ton startet einen „Burst“ (schnell, dann ruhiger); MORPH hoch = Supersaw-artig | [ ] |

- [ ] ENV DEST SHP (oder LFO DEST SHP) bewegt bei allen fünf TIMB.
- [ ] Keine Klicks beim Anschlagen, beim Loslassen, beim Wechsel der Engine auf einer Spur.
- [ ] Lautstärke der fünf untereinander und gegen ANALOG / FM6 ungefähr passend (sie spielen jetzt in Plaits' eigenen
      Verhältnissen: WSHAPE 0,7, HRMNC 0,8, WTABLE 0,6, PRTCL und SWARM durch Plaits' Limiter).

## 4. CPU messen (das Wichtigste)

MENU > SYSTEM > INFO > CPU, **8 Töne gehalten** auf einer Spur, sonst nichts. Bisher gemessen: 4 Töne WSHAPE 39 %,
8 Töne 64 % (vor dem Sparen bei AUX 0).

| Engine / Preset | AUX | erwartet | gemessen |
| --- | --- | --- | --- |
| WSHAPE SOFT FOLD | 0 | ~50 % | |
| HRMNC BRIGHT LEAD | 0 | ~66 % | |
| HRMNC DRAWBARS | 127 | ~52 % | |
| HRMNC AIR PAD | 30 | ~80 % | |
| PRTCL DUST CLOUD | 0 | ~55 % | |
| WTABLE WAVE SCAN | 0 | ~65 % | |
| SWARM SAW CLOUD | 0 | ~60 % | |

- [ ] Über 85 % nimmt die Firmware Stimmen weg statt zu knacken: einmal mit 8 Tönen HRMNC AIR PAD + Drums auf einer
      zweiten Spur probieren. Knackt es? Fallen Töne hörbar weg?
- [ ] Wenn eine Engine deutlich über 85 % liegt: mir die Zahl schicken, dann bekommt sie eine Stimmengrenze (wie FM6 6).

## 5. Alte Sounds

Projekte und User-Presets mit den entfernten Engines laden als erstes Preset des Nachfolgers (die Steps und der Rest
des Sounds bleiben):

| alt | lädt als |
| --- | --- |
| PHASE | WSHAPE SOFT FOLD |
| VOICE | WTABLE WAVE SCAN |
| TRIO | HRMNC DRAWBARS |
| SLICE | SWARM SAW CLOUD |
| DBOX | DRUM |

- [ ] Ein altes Projekt laden (falls noch eins mit diesen Engines da ist): lädt, spielt, nichts hängt.
- [ ] Favoriten (Sterne) der übrigen Engines sind noch da.

## 6. Offene Entscheidungen (für dich)

- [ ] **HRMNC / SWARM Stimmengrenze:** erst nach der CPU-Messung.
- [ ] **SWARM Burst:** jeder Ton startet einen Burst (wie Plaits mit Trigger, wie am Grind). Alternative: der Schwarm
      läuft gleichmäßig durch (besser für lange Flächen). Sag Bescheid, wenn du das lieber hättest.
- [ ] **Presets nach Gehör:** Die 40 neuen Presets habe ich nach den Parametern gebaut, nicht nach Gehör. Was nicht
      gut klingt, sag mir, oder speichere es als User-Preset und nenn mir die Werte.
- [ ] **J-6 Chord Mode:** gibt es in dieser Firmware nicht (siehe unten). Willst du ihn?
- [ ] **Flash:** noch 14,8 KB frei. Für größere neue Funktionen wird es eng; die Sample-Sets (104 KB) wären der große
      Brocken, falls einmal Platz gebraucht wird.

## 7. Akkord-Funktionen, die es gibt

Kein J-6-Modus, aber drei andere (alle aus früheren Durchgängen):

- **SCL > CHORD (CHRD, VOIC):** eine Taste spielt einen Akkord, entweder in der Tonart (DIA3, DIA7) oder einen festen
  Typ (MAJ MIN DOM7 MAJ7 MIN7 SUS4 POW); VOIC die Lage (CLOSE OPEN INV1 INV2 +OCT).
- **SCL > ORCHID:** die Akkord-Tastatur des Telepathic Instruments Orchid (links Typ und Erweiterungen, rechts die
  Grundtöne, LOCK).
- **GLO > KEY:** eine Akkordfolge über die Patterns, wie der Key-Track des OP-Z.

Was der J-6 anders macht: 100 fertige **Chord Sets** (je 16 Akkorde einer Stilrichtung auf den Tasten), dazu
Voicing-Varianten. So etwas bräuchte ~3–7 KB für die Akkordtabellen; das passt noch in den freien Flash.

## 8. Erledigt in diesem Durchgang

- [x] WSHAPE sparsamer (AUX 0: Plaits' zweiter Ausgang wird nicht gerechnet, OUT bitgleich)
- [x] HRMNC (Plaits Additive), bitgleich mit Plaits
- [x] PRTCL (Plaits Particle) mit Diffusor pro Part, Delays auf 7/10 gekürzt, bitgleich mit Plaits (volle Längen)
- [x] WTABLE (Plaits Wavetable) mit 8-Bit-Tabellen und Wellen-Cache pro Part, bitgleich mit Plaits (16-Bit-Tabellen)
- [x] SWARM (Plaits Swarm), bitgleich mit Plaits
- [x] Pegel aller fünf wie in Plaits
- [x] alle Host-Tests, Golden-Renders (nur die Plaits-Engines neu), Web-Editor, Doku
