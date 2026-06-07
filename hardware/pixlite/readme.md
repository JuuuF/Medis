# PixLite-Mapping

Mapping-Übersicht für PixLite 16 Mk.II

## Software-Setup

Programm: [Advatek Assistant 2](https://www.advateklighting.com/downloads/software)

### Output-Konfiguration

Advatek-Assistant: `PixLite -> Control -> Ethernet Ctrl -> Advanced`

| Output    | Start Universe | Start Channel | Num Pixels | Null Pixels | Group |
| --------- | -------------- | ------------- | ---------- | ----------- | ----- |
| Output 1  | 1              | 1             | 1020       | 0           | 1     |
| Output 2  | 7              | 1             | 1020       | 0           | 1     |
| Output 3  | 13             | 1             | 1020       | 0           | 1     |
| Output 4  | 19             | 1             | 1020       | 0           | 1     |
| Output 5  | 25             | 1             | 1020       | 0           | 1     |
| Output 6  | 31             | 1             | 1020       | 0           | 1     |
| Output 7  | 37             | 1             | 1020       | 0           | 1     |
| Output 8  | 43             | 1             | 1020       | 0           | 1     |
| Output 9  | 49             | 1             | 1020       | 0           | 1     |
| Output 10 | 55             | 1             | 1020       | 0           | 1     |
| Output 11 | 61             | 1             | 1020       | 0           | 1     |
| Output 12 | 67             | 1             | 1020       | 0           | 1     |
| Output 13 | 73             | 1             | 1020       | 0           | 1     |
| Output 14 | 79             | 1             | 1020       | 0           | 1     |
| Output 15 | 85             | 1             | 1020       | 0           | 1     |
| Output 16 | 91             | 1             | 1020       | 0           | 1     |

## Pixlite-Mapping

Der PixLite ist durch die [Output-Konfiguration](#output-konfiguration) so vorbereitet, dass jeder Output etwa 6 Universen an LEDs ansteuern kann (1020 RBG-LEDs * 3 Kanäle = 3060 Kanäle; 3060 Kanäle / 512 Kanäle/Universum $\approx$ 5.98 Universen). Diese werden innerhalb von Resolume auf Subnetzte und Universen gemappt.

Der PixLite kann 96 Universen auf 16 Outputs (im normalen Modus, nicht extended) ansteuern: 6 Universen $\times$ 16 Outputs = 96 Universen.

Innerhalb Resolumes werden Universen wie folgt auf Subnetze unterteilt:

| DMX-Universum | Resolume Subnetz | Subnetz-Universum |
| ------------- | ---------------- | ----------------- |
| 1-16          | 0                | 0-15              |
| 17-32         | 1                | 0-15              |
| 33-48         | 2                | 0-15              |
| 49-64         | 3                | 0-15              |
| 65-80         | 4                | 0-15              |
| 81-96         | 5                | 0-15              |

Es ergibt sich folgendes Mapping zwischen den PixLite-Outputs, Resolume und DMX-Universen:

| PixLite-Output-Bank | Resolume Start          | Resolume Ende           | DMX-Universen |
| ------------------- | ----------------------- | ----------------------- | ------------- |
| 1                   | Subnetz 0, Universum 0  | Subnetz 0, Universum 5  | 1-6           |
| 2                   | Subnetz 0, Universum 6  | Subnetz 0, Universum 11 | 7-12          |
| 3                   | Subnetz 0, Universum 12 | Subnetz 1, Universum 1  | 13-18         |
| 4                   | Subnetz 1, Universum 2  | Subnetz 1, Universum 7  | 19-24         |
| 5                   | Subnetz 1, Universum 8  | Subnetz 1, Universum 13 | 25-30         |
| 6                   | Subnetz 1, Universum 14 | Subnetz 2, Universum 3  | 31-36         |
| 7                   | Subnetz 2, Universum 4  | Subnetz 2, Universum 9  | 37-42         |
| 8                   | Subnetz 2, Universum 10 | Subnetz 2, Universum 15 | 43-48         |
| 9                   | Subnetz 3, Universum 0  | Subnetz 3, Universum 5  | 49-54         |
| 10                  | Subnetz 3, Universum 6  | Subnetz 3, Universum 11 | 55-60         |
| 11                  | Subnetz 3, Universum 12 | Subnetz 4, Universum 1  | 61-66         |
| 12                  | Subnetz 4, Universum 2  | Subnetz 4, Universum 7  | 67-72         |
| 13                  | Subnetz 4, Universum 8  | Subnetz 4, Universum 13 | 73-78         |
| 14                  | Subnetz 4, Universum 14 | Subnetz 5, Universum 3  | 79-84         |
| 15                  | Subnetz 5, Universum 4  | Subnetz 5, Universum 9  | 85-90         |
| 16                  | Subnetz 5, Universum 10 | Subnetz 5, Universum 15 | 91-96         |

Sind nun beispielsweise Pixel an Output-Bank 6 angeschlossen, können diese innerhalb Resolumes durch Adressierung des Lumiverses an Subnetz 1, Universum 14 angesteuert werden. Innerhalb des DMX-Netzwerks können diese beginnend bei DMX-Universum 31 angesteuert werden.

Wichtig ist dabei zu beachten, ob DMX-Universen bei 0 oder 1 beginnen zu zählen. Oben genannte Mappings gehen von einer 1-Indizierung der DMX-Universen aus (bedeutet: Das erste Universum hat Index 1, das zweite Index 2 etc.; bei 0-Indizierung hat das erste Universum Index 0, das zweite Index 1 etc.).
