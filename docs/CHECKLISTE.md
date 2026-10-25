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

### Stimmen nach CPU (neu)

- [ ] 4 Parts mit je 5 Noten (wie gemessen): Es klingen jetzt mehr als 8 Noten (erwartet 9–10 mit den Plaits-Engines),
      die CPU geht bis etwa 75–80 %, es knackt nicht.
- [ ] Dasselbe mit ANALOG oder LOFI auf den Parts: deutlich mehr Stimmen.
- [ ] Ein großer Akkord auf allen Parts gleichzeitig: kein Aussetzer.

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
- [ ] **J-6 Chord Mode (SCL > CSET):** ist wieder drin (siehe unten); auf dem Gerät durchprobieren.
- [x] **Flash:** FLUTE und SAX sind raus (63 KB frei, siehe Abschnitt 9).

## 7. Akkorde: SCL > CSET (J-6)

SCL > CSET (aus dem Branch `claude/cset-chords`, jetzt zusammengeführt) **ersetzt SCL > ORCHID**: die 100 Chord Sets
des Roland J-6, jede Taste ein Akkord des gewählten Sets. Ein Projekt mit ORCHID an lädt mit CSET aus.

- [ ] SCL dreimal: die Seite heißt CSET. KNOB 1 ON, KNOB 2 das Set (1 .. 100), KNOB 3 springt nach Stilrichtung.
- [ ] F3 .. E4 spielen die Akkorde des Sets in der Lage der J-6-Liste; Oktavtasten und TRN verschieben sie.
- [ ] Die Seite zeigt den zuletzt gespielten Akkord. ON macht die Spur POLY, OFF gibt die vorige VOICE zurück.
- [ ] STEP und Live-Aufnahme schreiben die Akkorde; MIDI-Noten spielen ebenfalls den Akkord ihrer Taste.
- [ ] Projekt speichern und laden: ON und Set bleiben pro Spur.

Außerdem gibt es weiterhin **SCL > CHORD** (CHRD, VOIC: eine Taste ein Akkord der Tonart oder ein fester Typ) und
**GLO > KEY** (eine Akkordfolge über die Patterns, wie beim OP-Z). Mit dem Merge kamen auch die **Legato-Aufnahme**
(gebundene Töne bleiben beim Live-Aufnehmen gebunden) und der **VDRUM-Kick ohne Rauschen** im Ausklingen zurück.

## 9. FLUTE und SAX als Dateien, Presets als Datei

- [ ] SAMPLE zeigt ab Werk nur noch PIANO; GRAIN CLOUD PAD und GLITCH nehmen jetzt PIANO als Quelle.
- [ ] Web-Editor > Samples > USR1 > **Slot file**: `samples/FLUTE.slot` (im Installer-ZIP) laden, SAMPLE SET USR1: klingt
      wie früher FLUTE (gehaltene Töne halten durch die Schleife). Dasselbe mit `SAX.slot`. USR1 hält eins von beiden.
- [ ] Web-Editor > Library: Sounds speichern, mit Tags (BASS, LEAD, PAD, ...) versehen, **Export presets**: die Datei
      schickst du mir, ich mache daraus Werks-Presets (`tools/import_presets.py`; gleicher Name ersetzt, neuer Name kommt
      dazu). Ein Werks-Preset hält: E0..E7, ATK DEC SUS REL, ENV>FLT, MONO; LFO, Matrix u. a. gehen dabei verloren.

- [ ] Web-Editor > **Projects > Presets on the device**: die User-Preset-Plätze U01 .. U32 wie Dateien. **Download**
      lädt eins als Datei, **Replace** (oder die Datei auf die Zeile ziehen) ersetzt einen Platz, **Upload files**
      schreibt mehrere (gleicher Name ersetzt, sonst ein leerer Platz), **Download all** alle in eine Datei. Danach am
      Gerät in PRESETS nachsehen: die neuen stehen neben den Werks-Presets.

## 10. Editor ohne Gerät

- [ ] Editor öffnen (Installer-Seite > Editor, oder der Link), oben **Ohne Gerät** klicken: nach ein paar Sekunden
      „Verbunden“, darunter die Tastatur-Zeile. Sprache oben rechts: Deutsch.
- [ ] Tastatur: A W S E D … spielt, Z / X Oktave, C / V Anschlag. Spur 1–4 oben wählen: die Tasten spielen diese Spur.
- [ ] Sound-Tab: Engine und Preset wählen, Regler drehen, man hört es sofort.
- [ ] Bibliothek: „Aktuellen Sound speichern“, mit Tags versehen, „Presets exportieren“ (die Datei an mich).
- [ ] Ein MIDI-Keyboard am Mac spielt den Sound mit (Chrome fragt nach MIDI-Zugriff).
- [ ] Am echten Gerät (Verbinden statt Ohne Gerät): die Tastatur spielt auch dort.
- [ ] FM-1 einmal **Verbinden**: unter Projekte > Presets am Gerät steht „Kopie dieser Seite: <Datum>“ (die User-Presets
      liegen jetzt im Browser). Trennen, **Ohne Gerät**: dieselben Presets sind da und klingen wie am FM-1 (Sound-Tab
      oder Bibliothek > User-Presets > Laden). Bearbeiten, mit SAVE am virtuellen Gerät bzw. im Editor ablegen.
- [ ] Wieder **Verbinden**: Button „Presets von „Ohne Gerät“ aufs FM-1“ bringt sie zurück (fragt vorher, Liste).

## 11. Werks-Presets der Reihe nach überarbeiten

- [ ] Sound-Tab: „Werks-Presets bearbeiten: Änderungen merken“ ist an (Standard).
- [ ] Ein Werks-Preset in der Liste wählen, Regler drehen: rechts steht „<NAME>: deine Fassung“. Das nächste Preset
      wählen, wieder drehen, … Zurück zum ersten: deine Fassung kommt wieder (in der Liste mit ✎ markiert).
- [ ] „Zurück zum Werkszustand“ verwirft die eigene Fassung des gewählten Presets.
- [ ] Am Ende „Bearbeitete exportieren“: eine Datei mit allen bearbeiteten Presets, die schickst du mir (sie ersetzen
      die Werks-Presets gleichen Namens). Die Fassungen liegen auch in der Bibliothek (Tag „edited“).

## 8. Erledigt in diesem Durchgang

- [x] WSHAPE sparsamer (AUX 0: Plaits' zweiter Ausgang wird nicht gerechnet, OUT bitgleich)
- [x] HRMNC (Plaits Additive), bitgleich mit Plaits
- [x] PRTCL (Plaits Particle) mit Diffusor pro Part, Delays auf 7/10 gekürzt, bitgleich mit Plaits (volle Längen)
- [x] WTABLE (Plaits Wavetable) mit 8-Bit-Tabellen und Wellen-Cache pro Part, bitgleich mit Plaits (16-Bit-Tabellen)
- [x] SWARM (Plaits Swarm), bitgleich mit Plaits
- [x] Pegel aller fünf wie in Plaits
- [x] alle Host-Tests, Golden-Renders (nur die Plaits-Engines neu), Web-Editor, Doku
