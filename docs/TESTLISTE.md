# M-Vave-OP1 auf dem FM-1: Testliste

Für den ersten Test auf echter Hardware. Abhaken, was klappt; bei allem anderen kurz notieren, was passiert ist (siehe
unten „So meldest du etwas“). Reihenfolge einhalten: die oberen Punkte zuerst, denn wenn dort etwas klemmt, sind
die unteren nicht aussagekräftig.

**Wichtig:** Hör beim Testen genau hin. **Knacken, Aussetzer oder Brummen** sind die wichtigsten Funde, denn das
kann nur das echte Gerät zeigen.

## A. Start und Grundlagen

- [ ] Startet nach dem Flash von selbst. Die Splash-Animation zeigt **JAN3000** im Cyber-Stil (flackernde Segmente, Farbversatz, kurze Glitches), dann
      **v0.8** mit blinkendem Cursor und darunter „based on Felucca“.
- [ ] HOME zeigt die vier Spuren.
- [ ] Ton aus Lautsprecher und Kopfhörer. MASTER regelt die Lautstärke.
- [ ] Akku-Anzeige oben rechts plausibel.
- [ ] Am Computer weiter als MIDI-Gerät sichtbar (USB-MIDI).
- [ ] Aus- und wieder einschalten: startet erneut sauber.

## B. Bedienelemente

- [ ] Alle 27 Tasten spielen. OCT− und OCT+ verschieben die Oktave.
- [ ] Alle Knöpfe öffnen ihre Seiten (FX, SCL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ).
- [ ] **SELECT blättert** die Fenster der Seite (am Ende wieder von vorn), das Tempo bleibt dabei gleich. Dieselbe
      Taste nochmal: zurück zu HOME. Auf HOME (und MIXER, CLOCK) ändert SELECT das Tempo.
- [ ] KNOB 1–4, SELECT, ALGORITHM (Spur wählen) und PRESETS reagieren ohne Sprünge.
- [ ] LEDs der Knöpfe und Tasten leuchten passend, nicht flackernd.
- [ ] Ein Knopf **gehalten** öffnet die Ebene (Layer). Ein Doppeltipp hält sie offen.

## C. Klang und Engines

- [ ] Jede der vier Spuren klingt mit ihrem Startsound.
- [ ] DRUM startet mit dem **808-Kit**.
- [ ] DRUM: EDIT, mit SELECT zu **SOUNDS**. Eine Taste (z. B. Snare) wählt den Sound, KNOB 1–4: TUNE, DECAY, TONE,
      SOUND (anderes Kit-Instrument). Nur dieser Sound ändert sich. Speichern und laden: Die Werte sind wieder da.
- [ ] PRESETS durchblättern: verschiedene Engines (FM6, ANALOG, SAMPLE …) klingen sauber.
- [ ] EDIT: die Reihen-Ansicht. Ein EDIT-Tipp wechselt die Reihe, die Werte ändern den Klang.
- [ ] **ANALOG2** (in PRESETS direkt nach ANALOG): EDIT zeigt vier Reihen (WAVE TUNE WAVE2 TUNE2 / CUT RES LFO RATE /
      CUT2 RES2 LFO2 RATE2 / VCE GLD GLMOD PRIO). LFO hoch und RATE ändern: Der Filter des Oszillators wabert hörbar.
      Alle acht Presets durchhören, auch mit vollen Akkorden. Kein Knacken.

## D. Sequencer

- [ ] STEP: Noten eingeben. Gestoppt fügt OCT+ eine Note hinzu, OCT− entfernt eine.
- [ ] STEP: KNOB 2 (NOTE) nach links bis zur tiefsten Note, noch eine Raste weiter: Die Karte zeigt **OFF**, der Step
      ist leer. Genauso nach rechts über die höchste Note hinaus. Aus OFF eine Raste drehen: die zuletzt gespielte
      Note ist wieder da. SAVE halten macht das OFF rückgängig.
- [ ] STEP: **EDIT löscht keinen Step mehr**, ein Tipp öffnet die EDIT-Seite (auch im Drum-Raster).
- [ ] PLAY und STOP. Tempo im CLOCK-Bereich ändern.
- [ ] **REC drücken startet nichts**, es macht nur scharf (Meldung REC ARMED: PRESS PLAY). Erst PLAY startet, mit
      Count-in, wenn COUNT-IN an ist (REC halten: COUNT-IN auf 1 BAR / 2 BARS).
- [ ] REC und live einspielen: Die Noten landen im Pattern.
- [ ] **Legato:** Einen Mono-Sound (VOICE LEG, z. B. SAW LEAD) aufnehmen und dabei überlappend spielen (die nächste
      Taste drücken, bevor die vorige losgelassen ist). Beim Abspielen startet die Hüllkurve an diesen Stellen nicht
      neu, genau wie beim Einspielen. Im Piano-Roll verbindet eine dunkle Linie die beiden Noten. Lässt du vorher
      los, startet die nächste Note wie bisher neu.
- [ ] REC halten + OCT−: Die Aufnahme wird rückgängig gemacht. REC halten + OCT+: Sie ist wieder da.
- [ ] **REC halten + F#5 (oberste schwarze Taste) = ZURÜCK:** macht die letzte Änderung rückgängig (eine Aufnahme,
      ein Step auf OFF, ein geladener Sound …). Die Taste atmet, solange es etwas zurückzunehmen gibt; nochmal
      gedrückt: ALREADY UNDONE. Auf der REC-Ebene steht unten „F#5 BACK“.
- [ ] DRUM, SEQ: Bei jedem Hi-Hat-Schlag (und jeder anderen Spur) blinkt die schwarze Taste der Spur kurz auf.
- [ ] **VDRUM im Raster:** eine Spur auf VDRUM, SEQ > STEP zeigt das Raster mit **6 Lanes** (BD SD CP CH OH TM).
      Schwarze Tasten 1–6 wählen die Lane und spielen sie, 7 und 8 bleiben dunkel und tun nichts. Weiße Tasten setzen
      Hits, ACC (schwarze Taste 9) gehalten setzt Akzente, die Lanes blinken beim Abspielen.
- [ ] **DBOX im Raster:** wie DRUM mit **8 Lanes**; schwarze Taste 8 (CB) ist die Cowbell (Taste B des Kits).
- [ ] VDRUM / DBOX: REC und live einspielen (auf dem Raster oder mit den normalen Tasten): Die Schläge landen als Hits.
- [ ] **VDRUM / DBOX Kick ohne Rauschen:** Kick mit Kopfhörern laut anhören (VDRUM INIT, TUBES, DUB; DBOX INIT,
      808-ISH): Das Ausklingen ist sauber, kein helles Zischen mehr darunter.
- [ ] VDRUM / DBOX: EDIT halten + schwarze Taste: die Lane stumm (Display „SNARE MUTED“), nochmal: wieder an.
- [ ] VDRUM / DBOX: SEQ halten: BEAT, RANDOM, COOK und die Lane-Werkzeuge (CLR, REV, FILL, RND der gewählten Lane).
- [ ] Ein altes Projekt mit VDRUM/DBOX-Noten laden: klingt **genau wie vorher**, die Schläge stehen jetzt als Hits im
      Raster. Dasselbe beim Wechseln auf eine alte Szene.
- [ ] Spur mit ALGORITHM wechseln, dann FX: FX beginnt wieder auf der ersten Karte (SEND).
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
- [ ] **KEY pro Szene:** Der Titel zeigt die Szene (KEY S1). In Szene 1 Am F C G setzen, mit SEQ + weiße Taste auf
      Szene 2: Dort steht zuerst dieselbe KEY (KEY S2). Szene 2 eine andere Folge geben (z. B. D G). Zwischen
      Szene 1 und 2 wechseln: Jede hat ihre eigene KEY. Beim Wechsel im Spielen startet die KEY der neuen Szene bei
      Takt 1.
- [ ] Projekt mit mehreren Szenen speichern und neu laden: Jede Szene hat wieder ihre KEY.

## I. SONG und SCENES

- [ ] Pattern-Ketten in SONG anlegen und abspielen.
- [ ] SEQ halten: Szenen wechseln am Pattern-Ende.
- [ ] SEQ halten + OCT+ lange halten (0,7 s) während es läuft: COPIED TO SCENE 2, die Musik läuft ohne Sprung oder
      Knacken weiter; beim Loslassen kein Szenenwechsel. Etwas ändern, dann SEQ + OCT−: Szene 1 ist unverändert,
      SEQ + OCT+ (kurz): die geänderte Kopie.

## J. Speichern (sehr wichtig)

- [ ] SAVE (PROJECT, Taste wählen, OCT+): Projekt speichern. Ausschalten, einschalten, OCT− lädt es: Alles ist wieder da (Noten, Sounds, FX
      TRACKS, Szenen, CHORUS-Typ, KEY).
- [ ] Ohne SAVE etwas ändern, ein paar Sekunden warten, aus- und einschalten: RESTORE LAST bringt den Stand zurück.
- [ ] SAVE öffnet PROJECT: F3 G3 A3 B3 wählen Projekt A–D (leuchten, kein Ton). OCT+ speichert (Name eingeben),
      OCT− lädt (mit Rückfrage). F#5 (höchste schwarze Taste) = NEW, dann OCT+: ein neues, leeres Projekt.
- [ ] Neues Projekt: T1 ein Bass, T3 ein Lead, T4 ein Drum-Kit, jedes Mal andere (T2 bleibt das Pad). Ebenso OCT− auf
      einem leeren Projekt (Rückfrage NEW PROJECT IN SLOT …?) und Einschalten mit RESTORE LAST OFF.
- [ ] Speichern blendet den Ton kurz aus und wieder ein, kein Knacken. Settings (MENU schließen) werden erst
      gespeichert, wenn nichts mehr klingt (höchstens 8 s später).

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
- [ ] MENU > AUDIO > SYNC OUT = PO, falls du einen Pocket Operator oder Volca hast: Stereokabel in den Line-In des PO
      (oder Stereo-auf-2×-Mono, links in den Sync-Eingang). PO auf SY2 oder SY4, **zuerst PLAY am PO drücken** (er
      wartet dann auf den Takt), dann PLAY am FM-1. Läuft synchron? Neu: 20 ms Puls, dazwischen bleibt der linke Kanal
      knapp unter 0 (nie ganz still), wie bei der App PocketSync.
- [ ] Falls nicht: dasselbe mit SYNC OUT = PO INV (der Puls andersherum). Bitte melden, welche Einstellung klappt.
      Klappt keine: Läuft der PO gar nicht, stolpert er oder läuft er im falschen Tempo?

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
