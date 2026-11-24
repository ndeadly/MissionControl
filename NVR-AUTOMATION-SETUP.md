# dynamicOS NVR – automatisierte CI

Dieser Branch ist für das vollständige dynamicOS-NVR-Projekt reserviert. Der Quellstand v18 muss noch in die Branch-Wurzel geladen werden (`nvr/`, `appliance/`, `android/`, `tests/`). Das Projektpaket `dynamicOS-NVR-v18-Automated-CI-Source.zip` wurde im Chat bereitgestellt.

Nach dem Hochladen löst der Workflow `continuous-nvr.yml` bei Pushes, manuell und täglich um 03:17 UTC automatische Regressionstests, einen Android-Debug-Build und den Versuch eines Linux-ISO-Builds aus. Builds sind **keine** Sicherheitsfreigabe. Installationen werden nicht automatisch vorgenommen. Die Linux-ISO ist aktuell keine verifizierte vollverschlüsselte Appliance.

Die GitHub-Automatisierung ist kein selbstständig programmierender Agent; neue Softwarefunktionen benötigen weiterhin Codeänderungen und Prüfung.
