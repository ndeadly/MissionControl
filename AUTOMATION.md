# dynamicOS NVR – automatisierte Builds

Die Workflowdatei `.github/workflows/continuous-nvr.yml` startet bei Änderungen, manuell oder jeden Tag um 03:17 UTC. Sie prüft Python und Shell, führt Regressionstests aus, baut eine Debug-APK und versucht ein Linux-ISO zu erstellen. Sie veröffentlicht die Artefakte nur nach erfolgreichem Build in GitHub Actions. Sie installiert **nichts** auf dem Mac mini. Eine Debug-APK ist keine release-signierte APK. Die ISO ist kein bestätigter verschlüsselter Installer; `appliance/build-iso.sh` kennzeichnet das Medium ausdrücklich als Live-/Installationsgrundlage.

## GitHub einrichten

1. Die Dateien dieses gesamten Projekts in den Branch `dynamicos-nvr-automation` von `kaivonderburg-create/MissionControl` kopieren, sodass `nvr/`, `android/`, `appliance/`, `tests/` und `.github/workflows/` direkt im Branch-Wurzelverzeichnis stehen.
2. Unter GitHub → Actions → `dynamicOS NVR continuous verification` → Run workflow einen ersten Lauf starten.
3. Die Jobs und Artefakte prüfen. Fehlgeschlagene Builds dürfen nicht als fertig bezeichnet werden.
4. Für Release-Signierung eine eigene, abgesicherte Signierpipeline erstellen. Keine Schlüssel in das Repository hochladen.

### Technische Grenzen

GitHub Actions arbeitet automatisiert, entwickelt aber nicht selbstständig neue Programme. Für autonome Codeänderungen wäre zusätzlich ein ausdrücklich konfigurierter Coding-Agent mit begrenzten GitHub-Berechtigungen, Reviewpflicht und isolierter Ausführung nötig. Die Pipeline ersetzt keine Hardware-Abnahme der ZOSI-Kameras und keinen Linux-EFI-/LUKS-Installationstest.
