# GeschwindigkeitsMessung

Arduino-Projekt zur Ermittlung von Geschwindigkeiten von Modellfahrzeugen.

## Details

* Arduino Uno
* Erfassung der Fahrzeuge über IR-Sensoren (Linienfolger)
* Anzeige der erfassten Daten über TM1637-Display

## Konfigurierbare Eigenschaften

Folgende Eigenschaften können angepasst werden:

* Länge der Messstrecke (Standard: 100cm)
* Modellmaßstab (Standard: 1/87)

## Anschlussplan

|Pin     |Bauteil                                       |
|--------|----------------------------------------------|
|2       |Sensor 1                                      |
|3       |Sensor 2                                      |
|4       |Reset-Taster                                  |
|5       |Taster zum Anzeige umschalten                 |
|6       |Taster zum Maßeinheit umschalten              |
|7       |Status-LED: Bereit                            |
|8       |Status-LED: Aktiv                             |
|9       |Status-LED: Fertig                            |
|10      |Anzeige-Modus-LED: Zeit                       |
|11      |Anzeige-Modus-LED: Geschwindigkeit            |
|12      |Anzeige-Modus-LED: Geschwindigkeit (skaliert) |
|13      |Maßeinheit-LED: m/s                           |
|14 (A0) |Maßeinheit-LED: km/h                          |
|15 (A1) |Display DIO-Pin                               |
|16 (A2) |Display CLK-Pin                               |
