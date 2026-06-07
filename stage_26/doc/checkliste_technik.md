# Checkliste Technik 2026

## Generelles

### Absprachen mit LJs

- Sobald LJs da sind...
- Konzept der Bühne darstellen (die wissen vermutlich noch nicht, wie die Bühne aussehen wird)
- eigene Lichteffekte vorstellen
  - Phönix:
    - 2x RGB (für LJs ansteuerbar)
    - Feuer-Effekt (nicht ansteuerbar)
  - Banner: 4x RBG
  - Kessel: ggf. 1x RBG (evtl. auch konstant grün - aber Kanäle dafür frei halten)
  - Quallen: X-mal 40x RBG, über Pixlite ansteuerbar
    - Kanäle mehr oder weniger fix
  - Flohfeuer:
    - Feuer-Effekt (nicht ansteuerbar)
    - Buzzer als Input-Trigger (über analogen Input)
      - siehe ##Flohfeuer

### Verkabelung von Boxen

- vor dem Anschließen an Strom mit Multimeter auf Kurzschluss prüfen
  - oder auf seine Verkabelung vertrauen und es nicht machen
  - ich bin einer gesunden Mischung aus beidem gefolgt
- wenn möglich, Zugentlastung durch Kabelbinder
  - ggf. in Boxen
  - ggf. vor Boxen am Layher
  - ggf. zwischendurch am Layher
  - Hintergrund:
    - es laufen überall Leute rum
    - Leute bleiben an Kabeln hängen
    - man selbst ist davon nicht ausgeschlossen
    - man selbst muss das aber wieder fixen

-------------------------------------------------

## Dementoren

### Dementoren vorbereiten

- Boxen von Quallen an Dementoren anbauen
- JEDE im Dementor eingebaute Box testen!
  - Setup mit Controller
  - 3-schrittiger Aufbau: Controller -> zu testende Box -> Output-Box
  - Controller gibt Daten im Demo-Modus aus
  - Daten + Strom werden auf Test-Buchse gelegt
  - Daten + Strom von Buckse in zu testende Box (= Input-Test)
  - Daten + Strom von Test-Box zu Output-Box (= Output-Test)
  - Resultat: beide Boxen müssen funktionieren!
    - evtl. wird Strom nicht weitergeleitet -> Verkabelung der Boxen checken (evtl. unterbrochenes Signal)
  - Stromversorgung extern!
    - wenn nicht extern, kann Netzteil vom Controller potenziell überlastet werden (ich bin nicht sicher, wie viel Leistung das Netzteil des Controllers hat; eventuell geht das auch)
- Kabel nach Test sorgfältig einrollen und in eigener Box einklicken, um Kabelsalat zu vermeiden

### Dementoren aufhängen

- Dementoren mit Hubsteiger / Rollgerüst zu Stahlkabeln fahren
- Dementoren mit Kabelbindern an Kabeln befestigen
- Kabel zwischen Dementoren ausrollen und zwischendurch mit Kabelbindern am Stahlkabel fixieren, dass sie nicht durchhängen
- darauf achten, dass Abstand zwichen Dementoren nicht länger als die Kabel ist

### Dementoren verkabeln: Strom

- Plan für das Aufstellen der Netzteile aufstellen
  - nicht mehr als 6 Dementoren je 20A Netzteil
  - Netzteil nicht zu weit von Dementoren entfernt
  - evtl. ist es möglich, Strom über weite Strecken zu schicken
    - kann aber in Voltage Drop resultieren
    - neues Aufstellen der Netzteile dauert dann wieder gewisse Zeit
- Netzteile in Boxen an ihren Positionen verteilen
- "Sternschaltungen" vorbereiten
  - = Verbindungsstücke über Netzteilen bzw. über Knotenpunkte an Crowdtürmen
  - bestehen aus Buchsen + Steckern, an denen Kabel entsprechend verzweigt werden
    - Stromversorgung zu Netzteilen
    - Durchschleifen von Daten

### Dementoren verkabeln: Daten

- Controller auf FOH aufstellen
- Daten-Ausgänge evtl. mit grün-schwarzen Decoder-Boxen versehen
  - sollte von der Strecke auch ohne möglich sein
  - musst schauen, was gut funktioniert und ob sie notwendig sind
- Daten-Ausgänge vom Controller an Buchsen für Dementor-Stecker anbinden
- Verkabelung durchtesten + an Strom anschließen
- hoffen, dass kein Netzteil zur Nebelmaschine wird

- Ethernet-Kabel zwischen Controller und GrandMA ziehen
- Kanal-Belegung mit LJs absprechen, sobald alles im Demo-Modus funktioniert

-------------------------------------------------

## Flohfeuer

### Feuer

- Boxen aufstellen:
  - Arduino an Netzteil anschließen
  - Stromkabel an Netzteil anschließen
- LEDs suchen + anbringen
  - schauen, ob 1 je Seite oder evtl. mehrere
- Stromversorgung der LEDs (unter dem Boden lang führen, dass niemand drüber läuft / stolpert)
- Datenkabel zwischen Arduino und LEDs
  - Arduino hat 4 Ausgänge
  - wenn aber mehr als 4 LED-Streifen verwendet werden, können auch Datenausgänge doppelt verwendet werden
  - geteilte Datenleitungen über beide Seiten verteilt
  - aber nicht alle Datenleitungen gespiegelt über beide seiten (dann wäre auf beiden Seiten der gleiche Effekt und das sieht dann vermutlich nicht gut aus)

### Buzzer

- wichtig: mit den LJs absprechen, ob sie damit schon Erfahrungen haben
  - ggf. können sie das einmal aus ihrer Sicht erklären oder weitere Einblicke gewähren
  - evtl. haben LJs passende Kabel dafür (spart dann Kabel für uns)
  - evtl. kann der Input kann unter der Bühne direkt angeschlossen werden
- Buzzer ggf. direkt am Pult testen, bevor er über lange Strecke verkabelt wird
- Buzzer aufstellen
- Buzzer verkabeln

-------------------------------------------------

## Phönix

### Vorbereitung

- LEDs an Phönix-Backboard anbringen (evtl. am Boxen; evtl. nachdem es hängt -> abhängig von Gegebenheiten vor Ort und Bauchgefühl)
  - Flügel- und Schweif-LEDs (Schläuche von letztem Jahr)
  - Körper-LEDs (single color)
  - Kopf-LEDs (single color)

### Stromversorgung des Phönix

- 20A Netzteil(e) für Controller + Körper- und Kopf-LEDs
  - vermutlich zusammen in einer Box
- 50A Netzteile für Feuer-LEDs
  - max. 2 Netzteile je Box
  - lieber nur eins je Box (Hintergrund: Sicherheit; die Dinger haben ordentlich Wumms)
  - Stromleitungen mit 4-quadrat-Kabeln über mehrere LED-Streifen daisy-chainen
    - Vcc/GND-Kabel parallel -> 3er-Wago:
      - Strom-Eingang
      - Strom-Abzweigung (zu LED)
      - Strom-Ausgang
  - grob durchrechnen, wie viele LED-Streifen an ein Netzteil angeschlossen werden können
    - abhängig von Länge der LEDs
    - grob gleichmäßig auf die 3 Ausgänge des Netzteils aufteilen
    - 50A / 1.5A/m = 33m (theoretisches Maximum); 33m * 0.8 = 27.5m (max. Auslastung von ~80%)

### Datenverteilung

#### Feuer-Daten

- Arduinos entweder in einzelne Boxen oder mit den Netzteilen zusammen
  - wenn mit Netzteilen: Trennung von Netzteilen und Srduinos, um Kurzschlüsse zu vermeiden
  - wenn Kurzschluss, dann bumm.
- Datenleitungen der Arduinos mit LEDs verbinden
  - ggf. Leitungen doppeln
  - bei Dopplung der Leitungen darauf achten, dass benachbarte LED-Streifen nicht gleiche Daten bekommen (ansonsten bringen die unterschliedlichen LEDs nicht viel)

#### Körper- und Kopf-Daten

- Kabeln von Controller zu Körper- und Kopf-LEDs ziehen
- ggf. mehrere Einspeisungen, wenn Voltage Drop offensichtlich
  - Anfang und Ende sollte vermutlich ausreichen
- LEDs mit DIP-Switches testen
- Startkanal an DIP-Switches einstellen
- DMX-Kabel an Controller anbinden (+ ggf. daisy-chainen)

-------------------------------------------------

## Kessel

- Netzteil + Controller in Box
  - grüner Controller, wenn er ansteierbar sein soll
  - grauer Controller, wenn nicht ansteuerbar
  - wenn nicht klar, ob ansteuerbar oder nicht, dann einfach grün
- Netzteil + Controller miteinander verkabeln
- Stromkabel an Netzteil anschließen
- LEDs per DIP-Switches testen

### Wenn ansteuerbar

- abgesprochenen Startkanal an DIP-Switches einstellen
- DMX-Kabel an Controller anschließen (+ ggf. weiter daisy-chainen)

### Wenn nicht ansteuerbar

- Farbe an Controller einstellen

-------------------------------------------------

## Schreibtisch / Banner

### Aufbau

- LEDs an Bannern anbringen
- ggf. Kabel an LEDs anlöten (oder vorhandene Kabel benutzen, wenn möglich)
  - auf Voltage Drop (= Helligkeitsverlust) achten, ggf. von beiden Seiten mit Strom versorgen
- vorbereitete Banner auf der Bühne anbringen

### Stromversorgung des Schreibtischs

- Netzteil + Controller (grüne Platine) in Box
- Netzteil + Controller miteinander verkabeln
- Stromkabel an Netzteil anschließen

### Daten

- Kabel zwischen Controller und Bannern ziehen
- LEDs per DIP-Switches testen
- abgesprochenen Startkanal an DIP-Switches einstellen
- DMX-Kabel an Controller anschließen (+ ggf. weiter daisy-chainen)
