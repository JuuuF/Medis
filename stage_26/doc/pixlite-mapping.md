# PixLite-Mapping

Mapping-Übersicht für PixLite 16 Mk.II

## Software-Setup

Programm: `Advatek Assistant 2`

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

Das Mapping der Universen auf die Pixlite-Outputs geschieht wie folgt:

| DMX-Universum | PixLite-Output-Bank |
| ------------- | ------------------- |
| 0-6           | 1                   |
| 7-12          | 2                   |
| 13-18         | 3                   |
| 19-24         | 4                   |
| 25-30         | 5                   |
| 31-36         | 6                   |
| 37-42         | 7                   |
| 43-48         | 8                   |
| 49-54         | 9                   |
| 55-60         | 10                  |
| 61-66         | 11                  |
| 67-72         | 12                  |
| 73-78         | 13                  |
| 79-84         | 14                  |
| 85-90         | 15                  |
| 91-96         | 16                  |

Dadurch ergibt sich folgendes Mapping zwischen den PixLite-Outputs und Resolume:

| PixLite-Output-Bank | Resolume Start          | Resolume Ende           |
| ------------------- | ----------------------- | ----------------------- |
| 1                   | Subnetz 0, Universum 0  | Subnetz 0, Universum 5  |
| 2                   | Subnetz 0, Universum 6  | Subnetz 0, Universum 11 |
| 3                   | Subnetz 0, Universum 12 | Subnetz 1, Universum 1  |
| 4                   | Subnetz 1, Universum 2  | Subnetz 1, Universum 7  |
| 5                   | Subnetz 1, Universum 8  | Subnetz 1, Universum 13 |
| 6                   | Subnetz 1, Universum 14 | Subnetz 2, Universum 3  |
| 7                   | Subnetz 2, Universum 4  | Subnetz 2, Universum 9  |
| 8                   | Subnetz 2, Universum 10 | Subnetz 2, Universum 15 |
| 9                   | Subnetz 3, Universum 0  | Subnetz 3, Universum 5  |
| 10                  | Subnetz 3, Universum 6  | Subnetz 3, Universum 11 |
| 11                  | Subnetz 3, Universum 12 | Subnetz 4, Universum 1  |
| 12                  | Subnetz 4, Universum 2  | Subnetz 4, Universum 7  |
| 13                  | Subnetz 4, Universum 8  | Subnetz 4, Universum 13 |
| 14                  | Subnetz 4, Universum 14 | Subnetz 5, Universum 3  |
| 15                  | Subnetz 5, Universum 4  | Subnetz 5, Universum 9  |
| 16                  | Subnetz 5, Universum 10 | Subnetz 5, Universum 15 |
