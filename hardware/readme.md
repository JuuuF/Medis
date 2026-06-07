# Technik für den Bühnenbau

## LEDs

### Kauf-Optionen der LEDs

Einzeln adressierbare LEDs:

- LED-Typ: `WS2811`
- [Aliexpress-Link](https://de.aliexpress.com/item/1005004289391906.html)
- Kauf-Paremeter:
  - White PCB
  - 5m
  - 60 LEDs/m
  - IP65

### Technische Daten

Spannung: 12V
Stromverbrauch: 9W/m = 1.5A/m

### Einbau der LEDs

Bei dem Einbau der LEDs auf der Bühne muss geplant werden, wie viel Strom verbraucht wird. Da die LEDs max. 1.5A/m verbrauchen, kann die maximale Gesamtlänge von LEDs an einem Netzteil wie folgt berechnet werden:

Beispiel: Netzteil mit 20A

20A / 1.5A/m = 13.3m LEDs

Dabei ist zu bedenken, dass Netzteile idealerweise bis 80% ihrer Leistung laufen sollten. Die Dadurch ergibt sich folgende Rechnung:

(20A * 0.8) / 1.5A/m = 16A / 1.5A/m = 10.7m LEDs.

Es sei dazu gesagt, dass diese Rechnung sehr pessimistisch ausgelegt ist und von einer konstanten Dauerbelastung ausgeht. Realistisch werden diese Verbräuche nicht erreicht, sodass knapp 11m LEDs je 20A Netzteil lediglich als grober Richtwert angenommen werden kann. Erfahrungsgemäß kann ein 20A Netzteil etwa 18m LEDs versorgen, mit gelegentlichem Flackern.

## Netzteile

### Kauf-Optionen der Netzteile

Bisher gekaufte general-purpose Netzteile:

- [Amazon-Link](https://www.amazon.de/dp/B08QDDY412)
- Spannung: 12V
- max. Strom: 20A

Für große Lasten:

- [Amazon-Link](https://www.amazon.de/dp/B0BQJML8ZD)
- Spannung: 12V
- max. Strom: 50A

Generell ist zu empfehlen, weiterhin 20A-Netzteile zu kaufen:

- sie liefern ausreichend Strom, um gute Mengen an LEDs zu versorgen
- sie sind preiswert
- sie besitzen Kurzschluss-Schutz
- sollte eins durchbrennen oder sonst wie den Geist aufgeben, kann es durch Ersatz-Netzteile schnell ausgetauscht werden
- die Planung ist einfacher, wenn man von gleicher Stromversorgung aller Netzteile ausgehen kann (mit Ausnahme der 50A-Chonker)
