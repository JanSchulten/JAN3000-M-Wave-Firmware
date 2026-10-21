# Erster Flash auf dem FM-1: Checkliste

Zum Abhaken, Schritt für Schritt. Lies einmal ganz durch, bevor du anfängst. Im Zweifel: **aufhören und
nachfragen**, nicht weiterprobieren.

## 0. Vorbereitung (einmal, ohne Gerät)

- [ ] Computer mit **Chrome oder Edge** (Safari und Firefox können kein Web MIDI mit SysEx).
- [ ] **USB-C-Datenkabel**, das sicher Daten überträgt. Viele Ladekabel können nur laden! Am besten das Kabel aus
      dem Karton oder eines, mit dem du schon Daten übertragen hast.
- [ ] Direkt am Computer einstecken, **kein USB-Hub**.
- [ ] **Python 3** auf dem Computer, für den lokalen Installer:
  - Mac: im Terminal `python3 --version` eingeben. Falls es fehlt, die vorgeschlagene Installation der
    Entwicklerwerkzeuge annehmen.
  - Windows: <https://www.python.org/downloads/> installieren und dabei **„Add python.exe to PATH“** anhaken.
- [ ] Den Installer herunterladen: `dist/M-Vave-OP1-Installer.zip` aus dem Repo (GitHub: Datei öffnen, „Download“)
      und **entpacken**.
- [ ] Optional, als Sicherheitsnetz für den seltenen Fall, dass das Gerät gar nicht mehr startet: ein
      **Seeed XIAO RP2040** (ein paar Euro) für [FM-1-Transporter](https://github.com/kurogedelic/FM-1-transporter).
      Hoffentlich brauchst du es nie.

## 1. Gerät auspacken: erst die Werks-Firmware prüfen (ca. 10 Minuten)

So schließen wir einen Hardware-Defekt aus, bevor irgendetwas geflasht wird.

- [ ] Gerät startet, Display ist klar, keine Pixelfehler.
- [ ] Alle **27 Tasten** spielen einen Ton.
- [ ] Alle **14 Knöpfe** reagieren (FX, SCL, ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC, OCT−, OCT+).
- [ ] Alle **7 Drehregler** reagieren (SELECT, ALGORITHM, PRESETS, KNOB 1–4) und **MASTER** ändert die Lautstärke.
- [ ] Ton aus dem **Lautsprecher** und aus dem **Kopfhörer**.
- [ ] Am Computer erscheint das FM-1 als **MIDI-Gerät**.
- [ ] Der **Akku lädt** und die Anzeige stimmt.
- [ ] Falls das Gerät eine Versionsnummer anzeigt: notieren.

Wenn hier schon etwas nicht stimmt: **nicht flashen**, sondern beim Händler reklamieren.

## 2. Vor jedem Flash

- [ ] **Akku voll** (oder das Gerät hängt am Strom). Ein leerer Akku mitten im Flash ist das größte Risiko.
- [ ] Computer am Strom, **Ruhezustand und Bildschirmsperre aus**.
- [ ] Andere Musik-Programme schließen, die das FM-1 belegen könnten (DAW, Editor, M-VAVE-Workbench).
- [ ] Nur **ein** Browser-Tab mit einem Installer offen.

## 3. Probelauf: erst das originale Felucca

Felucca läuft schon auf vielen Geräten. Klappt das, wissen wir, dass Flashen auf *deinem* Gerät funktioniert.

- [ ] Felucca-Installer öffnen: <https://hugelton.github.io/Felucca/webapp/installer/>.
- [ ] FM-1 anschließen, **Install** klicken, MIDI-Zugriff erlauben.
- [ ] Warten, bis „done“ erscheint und das FM-1 neu startet. **Nichts abziehen.**
- [ ] Felucca startet und spielt.
- [ ] **Zurück zur V15**, nach Schritt 6 („Zurück zur Werks-Firmware“). Das FM-1 startet wieder wie ab Werk.

Klappt das alles: Der Weg hin und zurück ist erprobt, weiter mit Schritt 4.

## 4. M-Vave-OP1 installieren

- [ ] Im entpackten Ordner starten:
  - Mac: **`START-Mac.command`** doppelklicken. Beim ersten Mal: Rechtsklick → Öffnen, falls macOS warnt.
  - Windows: **`START-Windows.bat`** doppelklicken.
- [ ] Der Browser öffnet `http://127.0.0.1:<Port>/webapp/installer/` (jeder Start nimmt einen freien Port; die Adresse
      steht im Starter-Fenster, ebenso der Name der Firmware-Datei). Falls nicht, die Adresse von Hand in Chrome oder
      Edge eingeben. Das Fenster mit dem Starter **offen lassen**, solange du installierst.
- [ ] Die Seite zeigt nach dem Laden **Felucca op1-…**: Die Kennung hinter „op1-“ muss zu der im Starter-Fenster
      passen. Bei einem Update: das neue ZIP in einen **neuen** Ordner entpacken und Starter-Fenster älterer Versionen
      schließen.
- [ ] FM-1 anschließen, **Install** klicken, MIDI-Zugriff (SysEx) erlauben.
- [ ] Der Fortschrittsbalken läuft: Prüfen, dann Schreiben. **Nichts abziehen, Tab nicht schließen.**
- [ ] „done“: Das FM-1 startet neu, M-Vave-OP1 erscheint.
- [ ] Weiter mit der **Testliste** (`docs/TESTLISTE.md`).

## 5. Während des Flashens: nie

- **Nie** das Kabel ziehen oder das FM-1 ausschalten.
- **Nie** den Browser-Tab schließen oder neu laden.
- **Nie** den Computer schlafen lassen.

## 6. Notfall: was tun, wenn …

**… der Flash abbricht** (Kabel, Fehler, Tab geschlossen):
- [ ] Ruhe bewahren, das FM-1 steht meist im **Update-Modus**.
- [ ] Gleichen Installer neu öffnen, gleiches Kabel, **Install** nochmal: Er setzt den unterbrochenen Schreibvorgang
      fort.

**… M-Vave-OP1 startet, aber sich falsch verhält** (Ton, Abstürze, hängt):
- [ ] Notieren oder filmen, was passiert (siehe „Was du mir schickst“).
- [ ] **Zurück zur Werks-Firmware:**
  - Im lokalen Installer „**Return to official V15**“ aufklappen.
  - „**Use the official V15 included here**“ klicken: Die V15 ist mitgeliefert, du musst nichts herunterladen.
  - „Back up and return to official V15“ klicken. Das sichert zuerst deine Musik als Datei auf den Computer.
- [ ] Alternativ die offizielle Seite von M-VAVE: <https://open.m-vave.com/fm1-workbench> (Firmware-Updater, V15).

**… das FM-1 gar nicht mehr startet** (dunkel oder hängt beim Start):
- [ ] **Nicht** wild weiterprobieren.
- [ ] Nach zwei Fehlstarts schaltet es von selbst in den Boot-Modus des Chips. Am Computer erscheint dann ein
      USB-Gerät `WL80UBOOT`.
- [ ] Von dort hilft nur **FM-1-Transporter** mit einem XIAO RP2040: erst den Flash sichern, dann die offizielle
      Firmware schreiben.
- [ ] Melde dich vorher, dann gehen wir das zusammen durch.

## Was du mir schickst, wenn etwas auffällt

- Was hast du gemacht (Seite, Knöpfe, Reihenfolge)?
- Was ist passiert, und was hättest du erwartet?
- Passiert es **jedes Mal**? Wie bekommt man es wieder hin?
- Am besten ein **Handy-Video mit Ton** oder ein Foto vom Display.
