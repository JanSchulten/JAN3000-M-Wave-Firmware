# M-Vave-OP1 auf dem FM-1: Testliste

Für den ersten Test auf echter Hardware. Abhaken, was klappt; bei allem anderen kurz notieren, was passiert ist (siehe
unten „So meldest du etwas“). Reihenfolge einhalten: die oberen Punkte zuerst, denn wenn dort etwas klemmt, sind
die unteren nicht aussagekräftig.

**Wichtig:** Hör beim Testen genau hin. **Knacken, Aussetzer oder Brummen** sind die wichtigsten Funde, denn das
kann nur das echte Gerät zeigen.

## A. Start und Grundlagen

- [ ] Startet nach dem Flash von selbst. Die Splash-Anzeige zeigt die Version.
- [ ] HOME zeigt die vier Spuren.
- [ ] Ton aus Lautsprecher und Kopfhörer. MASTER regelt die Lautstärke.
- [ ] Akku-Anzeige oben rechts plausibel.
- [ ] Am Computer weiter als MIDI-Gerät sichtbar (USB-MIDI).
- [ ] Aus- und wieder einschalten: startet erneut sauber.

## B. Bedienelemente

- [ ] Alle 27 Tasten spielen. OCT− und OCT+ verschieben die Oktave.
- [ ] Alle Knöpfe öffnen ihre Seiten (FX, SCL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ).
- [ ] KNOB 1–4, SELECT, ALGORITHM (Spur wählen) und PRESETS reagieren ohne Sprünge.
- [ ] LEDs der Knöpfe und Tasten leuchten passend, nicht flackernd.
- [ ] Ein Knopf **gehalten** öffnet die Ebene (Layer). Ein Doppeltipp hält sie offen.

## C. Klang und Engines

- [ ] Jede der vier Spuren klingt mit ihrem Startsound.
- [ ] DRUM startet mit dem **808-Kit**.
- [ ] PRESETS durchblättern: verschiedene Engines (FM6, ANALOG, SAMPLE …) klingen sauber.
- [ ] EDIT: die Reihen-Ansicht. Ein EDIT-Tipp wechselt die Reihe, die Werte ändern den Klang.

## D. Sequencer

- [ ] STEP: Noten eingeben. Gestoppt fügt OCT+ eine Note hinzu, OCT− entfernt eine.
- [ ] PLAY und STOP. Tempo im CLOCK-Bereich ändern.
- [ ] REC und live einspielen: Die Noten landen im Pattern.
- [ ] DETAIL und AUTOMATION (Knopfbewegungen aufnehmen) funktionieren.
- [ ] Ratchets und Chance hörbar.

## E. ARP

- [ ] ARP: eine Seite mit beiden Reihen, unten die Klaviatur.
- [ ] Tasten halten: Die Arp-Töne leuchten mit, der aktuelle Ton in der Akzentfarbe.
- [ ] ARP-Tipp wechselt die bearbeitete Reihe (MODE RATE OCT GATE ↔ SWING PROB HOLD ORDER).

## F. SCL und ORCHID

- [ ] SCL: Grundton und Tonleiter ändern, die Tasten folgen.
- [ ] SCL > ORCHID einschalten: F3–C#4 sind Akkordtasten, ab D4 spielen Grundtöne Akkorde. Der Akkordname
      erscheint.

## G. Effekte (FX)

- [ ] FX-Karte 1 **SEND**: vier Fader, sie regeln die Sends hörbar.
- [ ] INSERT: Typen durchschalten, MIX.
- [ ] CHORUS: TYPE CHORUS / FLANGER / VIBRATO, RATE, DEPTH, FDBK. Bei FLANGER mit viel FDBK den „Jet“ hören. Nach
      dem Stoppen **kein Brummen**.
- [ ] DELAY: TIME FDBK MIX. TYPE CLOUD (granular) klingt.
- [ ] REVERB: ROOM, SPRING, HALL, **SHIMMER**. SHIMMER steigt in Oktaven auf und **klingt vollständig aus**.
- [ ] SLICER, FX TRACKS.
- [ ] FX **halten** und weiße Tasten: Punch-in-Effekte (REPEAT, LPF, TAPE STOP …).
- [ ] Die Pedal-Grafiken sehen gut aus. Animationen laufen flüssig.

## H. KEY (GLO)

- [ ] GLO öffnet KEY. Die Tasten machen hier **keinen Ton**.
- [ ] Gestoppt: A, F, C, G tippen setzt Takt 1–4 (Am F C G).
- [ ] KNOB 4 auf ON, PLAY: Ein Pattern in C folgt den Akkorden.
- [ ] Während es läuft: Eine Taste wechselt die Tonart sofort (LIVE).
- [ ] REC auf der KEY-Seite: Die Tasten werden in die Takte geschrieben (REC in Rot).

## I. SONG und SCENES

- [ ] Pattern-Ketten in SONG anlegen und abspielen.
- [ ] SEQ halten: Szenen wechseln am Pattern-Ende.

## J. Speichern (sehr wichtig)

- [ ] SAVE: Projekt speichern. Ausschalten, einschalten, Projekt laden: Alles ist wieder da (Noten, Sounds, FX
      TRACKS, Szenen, CHORUS-Typ, KEY).
- [ ] Ohne SAVE etwas ändern, ein paar Sekunden warten, aus- und einschalten: RESTORE LAST bringt den Stand zurück.
- [ ] Während des Speicherns kurzes Stocken im Ton ist normal (~40 ms). Mehr nicht.

## K. Last-Test (sehr wichtig)

5 Minuten, so viel wie möglich gleichzeitig:

- [ ] Alle vier Spuren laufen, eine mit vollen Akkorden (POLY) und ARP.
- [ ] REVERB SHIMMER, DELAY CLOUD, CHORUS FLANGER mit FDBK, INSERT an.
- [ ] Dazu FX halten und Punch-ins drücken, KEY live wechseln.
- [ ] **Kein Knacken, keine Aussetzer**, das Display reagiert weiter flüssig.
- [ ] Falls es knackt: notieren, **welche Kombination** es auslöst (dann entlaste ich gezielt).

## L. MIDI und Sync

- [ ] USB-MIDI: Noten vom Computer spielen das FM-1. Das FM-1 sendet Noten an den Computer.
- [ ] Externe MIDI-Clock über USB: Das FM-1 folgt dem Tempo, Start und Stop.
- [ ] TRS-MIDI IN, falls du ein Kabel hast: Noten kommen an.
- [ ] MENU > AUDIO > SYNC OUT = PO, falls du einen Pocket Operator oder Volca hast: Stereo-auf-2×-Mono-Kabel, links in
      den Sync-Eingang. Läuft synchron?

## M. Dauer und Akku

- [ ] 30 Minuten spielen: keine Hänger, Gerät wird nicht auffällig warm.
- [ ] Akku-Anzeige sinkt plausibel.

## N. Rückweg

- [ ] Zum Schluss einmal **zurück zur V15** nach `ERSTER_FLASH.md`, Schritt 6, und wieder auf M-Vave-OP1. So weißt
      du, dass der Rückweg auf deinem Gerät funktioniert.

## So meldest du etwas

- **Wo:** Seite, Spur, welche Einstellung.
- **Was:** was passiert ist und was du erwartet hättest.
- **Wie oft:** jedes Mal? Wie löst man es aus?
- **Beweis:** Handy-Video **mit Ton** oder ein Foto vom Display.
