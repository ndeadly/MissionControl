"""dynamicOS / NVR — native macOS desktop security center."""
from __future__ import annotations
import datetime as dt
import io
import json
import os
import socket
import ipaddress
from pathlib import Path
import secrets
import subprocess
import sys
import time
import webbrowser
from urllib.parse import urlsplit
import cv2
# Qt 5.15 on Catalina 10.15; Qt 6 on supported modern macOS versions.
try:
    from PySide6.QtCore import Qt, QThread, Signal, QTimer, QBuffer, QByteArray, QIODevice, QUrl
    from PySide6.QtGui import QImage, QPixmap, QDesktopServices
    from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
     QGridLayout, QLabel, QPushButton, QLineEdit, QDialog, QFormLayout, QDialogButtonBox,
     QMessageBox, QInputDialog, QScrollArea, QFrame, QComboBox, QFileDialog, QSpinBox,
     QTabWidget, QStackedWidget, QTableWidget, QTableWidgetItem, QHeaderView, QCheckBox, QSlider)
except ImportError:
    from PySide2.QtCore import Qt, QThread, Signal, QTimer, QBuffer, QByteArray, QIODevice, QUrl
    from PySide2.QtGui import QImage, QPixmap, QDesktopServices
    from PySide2.QtWidgets import (QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
     QGridLayout, QLabel, QPushButton, QLineEdit, QDialog, QFormLayout, QDialogButtonBox,
     QMessageBox, QInputDialog, QScrollArea, QFrame, QComboBox, QFileDialog, QSpinBox,
     QTabWidget, QStackedWidget, QTableWidget, QTableWidgetItem, QHeaderView, QCheckBox, QSlider)
from zosi_lan import discover_onvif, scan_local_subnet, rtsp_candidates, probe_rtsp, private_ip, redact_url
from encrypted_media import decrypt_chunks, encrypt_stream, KeyProvider, VaultError
from backend import (load_config, save_config, new_auth, verify_password, valid_stream_url,
                     RecordingManager, EventStore, safe_name, ensure_private, recording_files,
                     ffmpeg_path, ffprobe_path, probe_camera_profile)

STYLE = r'''
QWidget { background:#090f19; color:#e4ecf8; font-family:"DejaVu Sans"; font-size:13px; }
QMainWindow, QDialog { background:#090f19; }
QFrame#panel, QFrame#statusPanel { background:#131e2c; border:1px solid #23374b; border-radius:16px; }
QFrame#sidebar { background:#101925; border-right:1px solid #243246; }
QFrame#hero { background:#152639; border:1px solid #285071; border-radius:17px; }
QLabel#brand { color:#e5f4ff; font-size:28px; font-weight:750; }
QLabel#eyebrow { color:#78bfdc; font-size:10px; font-weight:700; }
QLabel#sectionHeading { font-size:20px; color:#f4f9ff; font-weight:700; }
QLabel#muted { color:#94a8bf; }
QLabel#metric { font-size:16px; font-weight:700; color:#a4dcf4; }
QPushButton { background:#1b3045; border:1px solid #35516b; border-radius:9px; padding:10px 13px; color:#e8f4ff; }
QPushButton:hover { background:#284865; border-color:#5891ae; }
QPushButton:disabled { color:#617389; }
QPushButton#nav { text-align:left; background:transparent; border:0; border-radius:9px; padding:14px 16px; font-weight:600; }
QPushButton#nav:checked { background:#1c3851; border-left:3px solid #6dd6ee; color:white; }
QPushButton#primary { background:#19739a; border:1px solid #56b7db; }
QPushButton#danger { background:#51303b; border:1px solid #864152; }
QLineEdit,QComboBox,QSpinBox { background:#101d2d; border:1px solid #35516b; border-radius:8px; padding:8px; }
QTableWidget { background:#111e2b; border:1px solid #22354a; border-radius:10px; gridline-color:#25394a; alternate-background-color:#152538; }
QHeaderView::section { background:#1a2c40; color:#c6dded; padding:10px; border:0; }
QScrollArea { border:0; }
'''


class EncryptedPlayback(QThread):
    frame=Signal(object)
    problem=Signal(str)
    def __init__(self,path):
        super().__init__();self.path=Path(path);self.proc=None
    def run(self):
        ffmpeg=ffmpeg_path()
        if not ffmpeg:self.problem.emit('FFmpeg fehlt');return
        try:
            keys=KeyProvider();keys.master()
            args=[ffmpeg,'-hide_banner','-loglevel','error','-f','matroska','-i','pipe:0',
                  '-an','-vf','scale=1920:1080:flags=bicubic','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
            self.proc=subprocess.Popen(args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL)
            def feed():
                try:
                    from backend import playback_file_lock
                    with playback_file_lock(self.path) as source:
                        for chunk in decrypt_chunks(source,keys):
                            if self.isInterruptionRequested():break
                            self.proc.stdin.write(chunk)
                except (VaultError,OSError,BrokenPipeError) as exc:
                    self.problem.emit('Aufzeichnung kann nicht entschlüsselt werden')
                finally:
                    try:self.proc.stdin.close()
                    except (OSError,ValueError):pass
            import threading
            worker=threading.Thread(target=feed,daemon=True);worker.start()
            size=1920*1080*3
            while not self.isInterruptionRequested():
                data=self.proc.stdout.read(size)
                if len(data)!=size:break
                self.frame.emit(QImage(data,1920,1080,1920*3,QImage.Format_RGB888).copy())
        except (VaultError,OSError) as exc:self.problem.emit('Schlüsselbund oder Decoder nicht verfügbar')
        finally:
            if self.proc:
                if self.proc.poll() is None:
                    self.proc.terminate()
                    try:self.proc.wait(timeout=2)
                    except subprocess.TimeoutExpired:self.proc.kill()
                if self.proc.stdout:self.proc.stdout.close()

class PlaybackWindow(QDialog):
    def __init__(self,path,parent=None):
        super().__init__(parent);self.setWindowTitle('dynamicOS NVR · Verschlüsselte Wiedergabe')
        layout=QVBoxLayout(self)
        self.screen=QLabel('Lade verschlüsselte Aufnahme …');self.screen.setMinimumSize(960,540)
        self.screen.setAlignment(Qt.AlignCenter);layout.addWidget(self.screen)
        self.player=EncryptedPlayback(path)
        self.player.frame.connect(lambda img:self.screen.setPixmap(QPixmap.fromImage(img)))
        self.player.problem.connect(lambda msg:self.screen.setText(msg))
        self.player.start()
    def closeEvent(self,event):
        self.player.requestInterruption();self.player.wait(3500)
        super().closeEvent(event)

class Login(QDialog):
    def __init__(self, cfg):
        super().__init__(); self.cfg = cfg; self.setup = cfg.get('auth') is None
        self.setWindowTitle('dynamicOS NVR | Anmeldung'); self.setMinimumWidth(450)
        box = QVBoxLayout(self); box.setContentsMargins(30,32,30,30); box.setSpacing(15)
        title = QLabel('dynamicOS  /  NVR');title.setStyleSheet('font-size:30px;font-weight:750;color:#b6e9ff');box.addWidget(title)
        box.addWidget(QLabel('VON DER 3URG DYNAMIC    •    SECURITY OPERATIONS CENTER'))
        box.addWidget(QLabel('Administratorzugang erstellen' if self.setup else 'Administrator anmelden'))
        self.user = QLineEdit();self.user.setPlaceholderText('Benutzername');box.addWidget(self.user)
        self.pwd = QLineEdit();self.pwd.setEchoMode(QLineEdit.Password);self.pwd.setPlaceholderText('Passwort (mind. 12 Zeichen bei Einrichtung)');box.addWidget(self.pwd)
        if not self.setup:self.user.setText(cfg['auth'].get('user','admin'))
        button=QPushButton('Zugang erstellen' if self.setup else 'Anmelden');button.setObjectName('primary');box.addWidget(button)
        button.clicked.connect(self.submit);self.pwd.returnPressed.connect(self.submit)
        self.attempts=0;self.blocked_until=0
    def submit(self):
        if time.monotonic() < self.blocked_until:
            QMessageBox.warning(self,'Sperre','Zu viele Versuche. Bitte später erneut versuchen.');return
        user=self.user.text().strip();password=self.pwd.text()
        if self.setup:
            try:self.cfg['auth']=new_auth(user,password)
            except ValueError as e:QMessageBox.warning(self,'Sicherheit',str(e));return
            save_config(self.cfg);self.accept()
        elif verify_password(self.cfg['auth'],user,password):self.accept()
        else:
            self.attempts+=1
            if self.attempts>=5:self.blocked_until=time.monotonic()+60;self.attempts=0
            QMessageBox.warning(self,'Anmeldung','Zugangsdaten ungültig.')


class Feed(QThread):
    frame = Signal(object);status = Signal(str);motion=Signal()
    def __init__(self, url, motion_on=False, sensitivity=18):
        super().__init__();self.url=url;self.active=True;self.motion_on=motion_on;self.sensitivity=sensitivity
    def run(self):
        previous=None;last_motion=0
        while self.active:
            cap=cv2.VideoCapture(self.url,cv2.CAP_FFMPEG)
            if not cap.isOpened():
                self.status.emit('OFFLINE');cap.release()
                for _ in range(15):
                    if not self.active:return
                    self.msleep(200)
                continue
            self.status.emit('LIVE')
            n=0
            while self.active:
                okay,frame=cap.read()
                if not okay:break
                n+=1
                if self.motion_on and n%5==0:
                    gray=cv2.cvtColor(cv2.resize(frame,(320,180)),cv2.COLOR_BGR2GRAY)
                    gray=cv2.GaussianBlur(gray,(15,15),0)
                    if previous is not None:
                        delta=cv2.absdiff(previous,gray)
                        changed=(delta > max(7,45-int(self.sensitivity))).mean()
                        if changed > .035 and time.monotonic()-last_motion>30:
                            last_motion=time.monotonic();self.motion.emit()
                    previous=gray
                if n%2==0:
                    rgb=cv2.cvtColor(frame,cv2.COLOR_BGR2RGB)
                    h,w,ch=rgb.shape
                    self.frame.emit(QImage(rgb.data,w,h,ch*w,QImage.Format_RGB888).copy())
                self.msleep(30)
            cap.release();previous=None
            if self.active:self.status.emit('VERBINDE …')
        self.status.emit('GESTOPPT')
    def stop(self):self.active=False


class AddCamera(QDialog):
    def __init__(self,cam=None,parent=None):
        super().__init__(parent);self.setWindowTitle('Kamera konfigurieren');self.setMinimumWidth(540)
        form=QFormLayout(self)
        self.name=QLineEdit(cam.get('name','') if cam else '')
        self.url=QLineEdit(cam.get('url','') if cam else '')
        self.url.setPlaceholderText('rtsp://benutzer:passwort@kamera-ip:554/stream1')
        self.enabled=QCheckBox('Kamera aktiviert');self.enabled.setChecked(cam.get('enabled',True) if cam else True)
        form.addRow('Kameraname',self.name);form.addRow('RTSP / HTTP(S)',self.url);form.addRow('',self.enabled)
        lan=QPushButton('ZOSI LAN · IP-Kamera finden / testen')
        form.addRow(lan);lan.clicked.connect(self.zosi_setup)
        quality=QPushButton('1080p-Hauptstream prüfen (FFprobe)')
        form.addRow(quality);quality.clicked.connect(self.check_quality)
        hint=QLabel('Kamera muss eine abrufbare Video-URL bereitstellen. Kamera-Zugangsdaten werden im macOS-Schlüsselbund gespeichert; keine Klartextpasswörter in settings.json.');hint.setWordWrap(True);form.addRow(hint)
        buttons=QDialogButtonBox(QDialogButtonBox.Save|QDialogButtonBox.Cancel);form.addRow(buttons)
        buttons.accepted.connect(self.validate);buttons.rejected.connect(self.reject)
    def check_quality(self):
        try:
            detail=probe_camera_profile(self.url.text().strip(),timeout=10)
            quality='1080p (Full HD) bestätigt' if detail['full_hd'] else 'KEIN Full HD: Hauptstream/Modell prüfen'
            QMessageBox.information(self,'Kameraqualität',
                f"{detail['width']} × {detail['height']} · {detail['codec']} · {detail['frame_rate']} FPS\n{quality}")
        except (RuntimeError,ValueError,OSError) as exc:
            QMessageBox.warning(self,'1080p Diagnose',str(exc))

    def zosi_setup(self):
        selection, ok = QInputDialog.getItem(self, 'ZOSI LAN', 'Suchmethode',
                    ['ONVIF-Geräte erkennen (LAN)', 'Kamera-IP direkt eingeben', 'Lokales /24-Netz nach Kamera-Ports durchsuchen'], 0, False)
        if not ok:return
        ip = ''
        if selection.startswith('ONVIF'):
            QApplication.setOverrideCursor(Qt.WaitCursor)
            try: found = discover_onvif(timeout=2)
            finally: QApplication.restoreOverrideCursor()
            if not found:
                QMessageBox.information(self,'ONVIF','Keine ONVIF-Kamera gefunden. Die direkte IP-Eingabe funktioniert auch ohne ONVIF.');return
            ip, ok = QInputDialog.getItem(self,'Gefundene IP-Kameras','LAN-IP auswählen',[v['ip'] for v in found],0,False)
            if not ok:return
        elif selection.startswith('Lokales'):
            subnet, ok = QInputDialog.getText(self,'LAN-Netz','Privates CIDR-Netz, z. B. 192.168.1.0/24')
            if not ok:return
            QApplication.setOverrideCursor(Qt.WaitCursor)
            try: found=scan_local_subnet(subnet.strip())
            except ValueError as e: QMessageBox.warning(self,'LAN-Suche',str(e));return
            finally: QApplication.restoreOverrideCursor()
            if not found: QMessageBox.information(self,'LAN-Suche','Keine Geräte mit passenden offenen Ports gefunden.');return
            ip,ok=QInputDialog.getItem(self,'LAN-Geräte','IP-Adresse wählen',[v['ip']+'  ·  Ports '+str(v['ports']) for v in found],0,False)
            if not ok:return
            ip=ip.split('  ·')[0]
        else:
            ip,ok=QInputDialog.getText(self,'ZOSI-Kamera','IPv4-Adresse der Kamera im LAN')
            if not ok:return
        ip=ip.strip()
        if not private_ip(ip):QMessageBox.warning(self,'LAN','Bitte eine private lokale IPv4-Adresse angeben.');return
        user, ok=QInputDialog.getText(self,'Kamerazugang','Benutzername der Kamera (optional)')
        if not ok:return
        pwd, ok=QInputDialog.getText(self,'Kamerazugang','Passwort der Kamera (optional)',QLineEdit.Password)
        if not ok:return
        binary=ffprobe_path()
        if not binary:
            QMessageBox.warning(self,'Abhängigkeit','FFprobe fehlt. Bitte FFmpeg installieren bzw. ins App-Bundle aufnehmen.');return
        QApplication.setOverrideCursor(Qt.WaitCursor)
        tested=[]
        try:
            for url in rtsp_candidates(ip,user,pwd):
                result=probe_rtsp(url,binary,timeout=4)
                tested.append((url,result))
                if result['ok']:break
        finally: QApplication.restoreOverrideCursor()
        good=next(((url,res) for url,res in tested if res['ok']),None)
        if good:
            self.url.setText(good[0])
            if not self.name.text().strip():self.name.setText('ZOSI '+ip)
            QMessageBox.information(self,'Kamera erreichbar','RTSP-Videostream erfolgreich verifiziert.\n'+good[1].get('details',''))
        else:
            QMessageBox.warning(self,'Kein Stream','Keiner der getesteten Standardpfade liefert ein verifiziertes Videobild.\nModell und Kamera-Firmware prüfen; gegebenenfalls Stream-URL manuell eingeben.')

    def validate(self):
        if not self.name.text().strip() or not valid_stream_url(self.url.text()):
            QMessageBox.warning(self,'Ungültige Angaben','Name und gültige Stream-URL angeben.');return
        self.accept()


class Tile(QFrame):
    def __init__(self,cam, manager, events, cfg):
        super().__init__();self.setObjectName('panel');self.cam=cam;self.manager=manager;self.events=events;self.cfg=cfg
        self.last_frame=None;self.setMinimumSize(350,280)
        lay=QVBoxLayout(self);lay.setSpacing(8)
        bar=QHBoxLayout();title=QLabel(cam['name']);title.setStyleSheet('font-size:16px;font-weight:650');bar.addWidget(title)
        self.status=QLabel('VERBINDE …');self.status.setObjectName('muted');bar.addWidget(self.status,0,Qt.AlignRight);lay.addLayout(bar)
        self.screen=QLabel('WARTE AUF VIDEOSIGNAL');self.screen.setAlignment(Qt.AlignCenter)
        self.screen.setStyleSheet('background:#040812;border:1px solid #1f2c3f;border-radius:10px;color:#7695b1');self.screen.setMinimumHeight(190)
        lay.addWidget(self.screen,1)
        buttons=QHBoxLayout();self.record=QPushButton('● Aufnahme');self.record.clicked.connect(self.toggle_record)
        if os.environ.get('DYNAMICOS_EXTERNAL_RECORDER') == '1':
            self.record.setEnabled(False);self.record.setToolTip('24/7 Aufnahme läuft im unabhängigen Systemdienst')
        shot=QPushButton('Schnappschuss');shot.clicked.connect(self.snapshot)
        buttons.addWidget(self.record);buttons.addWidget(shot);lay.addLayout(buttons)
        self.feed=Feed(cam['url'],cfg.get('motion_enabled',False),cfg.get('motion_sensitivity',18))
        self.feed.frame.connect(self.draw);self.feed.status.connect(self.status.setText);self.feed.motion.connect(self.motion_event);self.feed.start()
    def draw(self,image):
        self.last_frame=image
        self.screen.setPixmap(QPixmap.fromImage(image).scaled(self.screen.size(),Qt.KeepAspectRatio,Qt.FastTransformation))
    def motion_event(self):self.events.log('Bewegung erkannt',self.cam['name'],'Lokale Bilddifferenzanalyse; keine Personenerkennung')
    def toggle_record(self):
        active=self.manager.toggle_manual(self.cam['id']);self.manager.tick()
        self.record.setText('■ Manuell stoppen' if active else '● Aufnahme')
    def snapshot(self):
        if self.last_frame is None:
            QMessageBox.information(self,'Schnappschuss','Aktuell kein Kamerabild vorhanden.');return
        dest=Path(self.cfg['snapshot_dir']);ensure_private(dest)
        file=dest/(safe_name(self.cam['name'])+'_'+dt.datetime.now().strftime('%Y%m%d_%H%M%S')+'_'+secrets.token_hex(3)+'.dsnap')
        buffer=QByteArray();qbuffer=QBuffer(buffer)
        if not qbuffer.open(QIODevice.WriteOnly) or not self.last_frame.save(qbuffer,'JPG'):
            QMessageBox.warning(self,'Schnappschuss','Bild konnte nicht kodiert werden.');return
        qbuffer.close()
        try:
            with file.open('xb') as output:
                os.chmod(file,0o600)
                encrypt_stream(io.BytesIO(bytes(buffer)),output,KeyProvider())
                output.flush();os.fsync(output.fileno())
            self.events.log('Verschlüsselter Schnappschuss',self.cam['name'],file.name)
        except (OSError,VaultError) as exc:
            file.unlink(missing_ok=True)
            QMessageBox.warning(self,'Schnappschuss','Verschlüsselung nicht möglich; Bild wurde nicht gespeichert.')

    def shutdown(self):
        self.feed.stop()
        if not self.feed.wait(2500):
            # Avoid premature object destruction while OpenCV is blocked inside a read.
            self.feed.wait(8000)


class Settings(QDialog):
    def __init__(self,cfg,parent=None):
        super().__init__(parent);self.cfg=cfg;self.setWindowTitle('dynamicOS NVR · Systemeinstellungen');self.setMinimumWidth(500)
        form=QFormLayout(self)
        self.mode=QComboBox();self.mode.addItem('Manuell','manual');self.mode.addItem('24/7 Daueraufzeichnung','continuous');self.mode.addItem('Zeitgesteuert','scheduled')
        idx=self.mode.findData(cfg.get('record_mode','continuous'));self.mode.setCurrentIndex(max(0,idx));form.addRow('Aufnahmemodus',self.mode)
        self.begin=QLineEdit(cfg.get('schedule_start','00:00'));self.end=QLineEdit(cfg.get('schedule_end','23:59'))
        form.addRow('Zeitplan Beginn HH:MM',self.begin);form.addRow('Zeitplan Ende HH:MM',self.end)
        self.hours=QSpinBox();self.hours.setRange(0,87600);self.hours.setValue(int(cfg.get('retention_hours',int(cfg.get('retention_days',14))*24)));form.addRow('Aufbewahrung (Stunden, 0 = unbegrenzt)',self.hours)
        self.limit=QSpinBox();self.limit.setRange(0,100000);self.limit.setValue(int(cfg.get('max_storage_gb',0)));form.addRow('Speicherlimit (GB, 0 = unbegrenzt)',self.limit)
        self.motion=QCheckBox('Lokale Bewegungsereignisse protokollieren');self.motion.setChecked(cfg.get('motion_enabled',False));form.addRow(self.motion)
        self.sensitivity=QSlider(Qt.Horizontal);self.sensitivity.setRange(1,40);self.sensitivity.setValue(int(cfg.get('motion_sensitivity',18)));form.addRow('Bewegungsempfindlichkeit',self.sensitivity)
        buttons=QDialogButtonBox(QDialogButtonBox.Save|QDialogButtonBox.Cancel);form.addRow(buttons)
        buttons.accepted.connect(self.save);buttons.rejected.connect(self.reject)
    def save(self):
        try:
            dt.time.fromisoformat(self.begin.text().strip());dt.time.fromisoformat(self.end.text().strip())
        except ValueError:QMessageBox.warning(self,'Zeitformat','Bitte Uhrzeiten im Format HH:MM angeben.');return
        self.cfg.update(record_mode=self.mode.currentData(),schedule_start=self.begin.text().strip(),schedule_end=self.end.text().strip(),
                        retention_hours=self.hours.value(),retention_days=(self.hours.value()+23)//24,max_storage_gb=self.limit.value(),motion_enabled=self.motion.isChecked(),
                        motion_sensitivity=self.sensitivity.value())
        save_config(self.cfg);self.accept()


from secure_server import create_app, NVRService, generate_certificate, cert_fingerprint
import threading
import uvicorn

class Main(QMainWindow):
    def __init__(self,cfg):
        super().__init__();self.cfg=cfg;self.events=EventStore();self.manager=RecordingManager(cfg,self.events);self.tiles=[]
        self.network_service=NVRService(config=self.cfg)
        self.network_service.recording=self.manager
        self.network_service.events=self.events
        self.network_server=None
        self.network_thread=None
        self.lan_host=None
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':
            self.launch_https()
        self.setWindowTitle('dynamicOS NVR | SECURITY COMMAND');self.resize(1500,930)
        root=QWidget();self.setCentralWidget(root)
        shell=QHBoxLayout(root);shell.setContentsMargins(0,0,0,0);shell.setSpacing(0)
        sidebar=QFrame();sidebar.setObjectName('sidebar');sidebar.setFixedWidth(252)
        sl=QVBoxLayout(sidebar);sl.setContentsMargins(16,27,16,20);sl.setSpacing(9)
        brand=QLabel('dynamicOS');brand.setObjectName('brand');sl.addWidget(brand)
        product=QLabel('NVR  /  SECURITY APPLIANCE');product.setObjectName('eyebrow');sl.addWidget(product)
        sl.addSpacing(32)
        self.navigation=[]
        content=QWidget();outer=QVBoxLayout(content);outer.setContentsMargins(26,23,26,18);outer.setSpacing(15)
        heading=QHBoxLayout();headline=QVBoxLayout();headline.setSpacing(4)
        heading_label=QLabel('SECURITY OPERATIONS CENTER');heading_label.setObjectName('eyebrow');headline.addWidget(heading_label)
        self.page_title=QLabel('Liveüberwachung');self.page_title.setObjectName('sectionHeading');headline.addWidget(self.page_title)
        heading.addLayout(headline);heading.addStretch()
        self.vault_state=QLabel('VAULT WIRD GEPRÜFT');self.vault_state.setObjectName('metric');heading.addWidget(self.vault_state)
        outer.addLayout(heading)
        hero=QFrame();hero.setObjectName('hero');hl=QHBoxLayout(hero);hl.setContentsMargins(18,14,18,14)
        brief=QVBoxLayout();brief.addWidget(QLabel('VON DER 3URG DYNAMIC  /  PRIVATE VIDEO SECURITY'))
        about=QLabel('Lokale Überwachung • Verschlüsseltes Archiv • Keine öffentliche Cloud');about.setObjectName('muted');brief.addWidget(about)
        hl.addLayout(brief);hl.addStretch()
        for label,method in [('＋ Kamera',self.add_cam),('Android koppeln',self.show_pairing)]:
            button=QPushButton(label);button.clicked.connect(method);hl.addWidget(button)
        outer.addWidget(hero)
        self.tabs=QStackedWidget();outer.addWidget(self.tabs,1)
        live=QWidget();lv=QVBoxLayout(live)
        self.counter=QLabel();self.counter.setObjectName('muted');lv.addWidget(self.counter)
        scroll=QScrollArea();scroll.setWidgetResizable(True);scroll.setFrameShape(QFrame.NoFrame)
        holder=QWidget();self.grid=QGridLayout(holder);self.grid.setSpacing(14);scroll.setWidget(holder);lv.addWidget(scroll,1)
        self.tabs.addWidget(live)
        recordings=QWidget();rv=QVBoxLayout(recordings);buttons=QHBoxLayout()
        refresh=QPushButton('Archiv aktualisieren');refresh.clicked.connect(self.update_recordings)
        openfolder=QPushButton('Archivverzeichnis');openfolder.clicked.connect(lambda:self.open_folder(self.cfg['record_dir']))
        for b in (refresh,openfolder):buttons.addWidget(b)
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':
            choose=QPushButton('Speicherort ändern');choose.clicked.connect(self.change_dir);buttons.addWidget(choose)
        buttons.addStretch();rv.addLayout(buttons)
        self.files=QTableWidget(0,3);self.files.setHorizontalHeaderLabels(['Aufnahme','Größe','Zeitpunkt'])
        self.files.horizontalHeader().setSectionResizeMode(0,QHeaderView.Stretch);rv.addWidget(self.files)
        play=QPushButton('Ausgewählte Aufnahme sicher abspielen');play.clicked.connect(self.play_recording);rv.addWidget(play)
        self.tabs.addWidget(recordings)
        eventpanel=QWidget();ev=QVBoxLayout(eventpanel)
        update=QPushButton('Sicherheitsereignisse aktualisieren');update.clicked.connect(self.update_events);ev.addWidget(update)
        self.event_table=QTableWidget(0,4);self.event_table.setHorizontalHeaderLabels(['Zeit','Kamera','Ereignis','Details'])
        self.event_table.horizontalHeader().setSectionResizeMode(3,QHeaderView.Stretch);ev.addWidget(self.event_table)
        self.tabs.addWidget(eventpanel)
        control=QWidget();cl=QVBoxLayout(control)
        description=QLabel('Kameras, autorisierte Geräte und Sicherheitseinstellungen.');description.setObjectName('muted');cl.addWidget(description)
        for label,action in [('Kamera hinzufügen',self.add_cam),('Kamera bearbeiten',self.edit_cam),('Kamera entfernen',self.remove_cam),('Android-Geräte verwalten',self.manage_devices),('NVR-Einstellungen',self.settings)]:
            btn=QPushButton(label);btn.clicked.connect(action);cl.addWidget(btn)
        cl.addStretch();self.tabs.addWidget(control)
        for index,(label,title) in enumerate([
            ('◉   Live','Liveüberwachung'),('▤   Videoarchiv','Verschlüsseltes Videoarchiv'),
            ('◇   Ereignisse','Sicherheitsereignisse'),('⚙   Verwaltung','Geräteverwaltung')]):
            btn=QPushButton(label);btn.setObjectName('nav');btn.setCheckable(True)
            btn.clicked.connect(lambda checked,i=index,t=title:self.navigate(i,t))
            sl.addWidget(btn);self.navigation.append(btn)
        self.navigation[0].setChecked(True)
        sl.addStretch();footer=QLabel('LOCAL-FIRST  ·  ENCRYPTED VAULT\nSYSTEM EDITION  /  12');footer.setObjectName('muted');sl.addWidget(footer)
        shell.addWidget(sidebar);shell.addWidget(content,1)
        foot=QHBoxLayout();self.status=QLabel('LOKALES SYSTEM · KEINE CLOUD-VERBINDUNG');self.status.setObjectName('muted');foot.addWidget(self.status);foot.addStretch()
        self.clock=QLabel();self.clock.setObjectName('muted');foot.addWidget(self.clock);outer.addLayout(foot)
        self.tabs.currentChanged.connect(self.tab_changed)
        self.refresh_tiles();self.update_recordings();self.update_events()
        self.timer=QTimer(self);self.timer.timeout.connect(self.tick);self.timer.start(5000);self.tick()
        self.check_vault_status()
        self.events.log('Anmeldung',details='Security Appliance gestartet')

    def navigate(self,index,title):
        self.tabs.setCurrentIndex(index);self.page_title.setText(title)
        for i,button in enumerate(self.navigation):button.setChecked(i==index)

    def check_vault_status(self):
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':
            self.vault_state.setText('ENTWICKLUNGSMODUS');return
        try:
            from security_policy import require_vault
            require_vault(self.cfg['record_dir'])
            from encrypted_media import KeyProvider
            KeyProvider().master()
            self.vault_state.setText('● VAULT AKTIV')
        except Exception:
            self.vault_state.setText('● VAULT GESPERRT')

    def launch_https(self, lan_host=None):
        """Local only until the administrator explicitly activates a LAN interface."""
        try:
            if self.network_server is not None:
                self.network_server.should_exit=True
                if self.network_thread:
                    self.network_thread.join(timeout=4)
                if self.network_thread and self.network_thread.is_alive():
                    raise RuntimeError('Vorheriger Netzwerkserver wurde nicht beendet')
            host=lan_host or '127.0.0.1'
            networks=[str(ipaddress.ip_network(host+'/24',strict=False))] if lan_host else []
            generate_certificate(hosts=('127.0.0.1',host))
            config=uvicorn.Config(create_app(self.network_service,allowed_networks=networks),
                host=host,port=8443,ssl_certfile=str(__import__('secure_server').CERT_FILE),
                ssl_keyfile=str(__import__('secure_server').KEY_FILE),
                access_log=False,log_level='warning',server_header=False)
            self.network_server=uvicorn.Server(config)
            self.network_thread=threading.Thread(target=self.network_server.run,daemon=True)
            self.network_thread.start()
            self.lan_host=lan_host
            self.network_error=''
        except Exception as exc:
            self.network_error=str(exc)
            self.network_server=None

    def show_pairing(self):
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
            # The GUI must never start an additional network listener.
            # A root-controlled API service owns the only HTTPS socket.
            from secure_server import CERT_FILE
            host=os.environ.get('DYNAMICOS_API_HOST','127.0.0.1')
            if host=='127.0.0.1':
                try:
                    api_env=Path('/etc/dynamicos-nvr/api.env').read_text()
                    for line in api_env.splitlines():
                        if line.startswith('DYNAMICOS_API_HOST='):
                            host=line.split('=',1)[1].strip().strip('\"')
                except OSError: pass
            if host in ('127.0.0.1','::1','localhost') or not CERT_FILE.is_file():
                QMessageBox.warning(self,'Android-Zugang nicht aktiv',
                    'Die separate dynamicOS-API ist noch nicht für eine freigegebene LAN-IP konfiguriert. '
                    'Ein Administrator muss /etc/dynamicos-nvr/api.env einrichten und dynamicos-api.service starten. '
                    'Es wird keine automatische Netzfreigabe vorgenommen.')
                return
            code=self.network_service.devices.create_pairing()
            QMessageBox.information(self,'Android sicher koppeln',
                f'API: https://{host}:8443\n\nEinmaliger Kopplungscode (5 Minuten): {code}\n\n'
                f'SHA-256-Zertifikatsfingerprint:\n{cert_fingerprint()}\n\n'
                'Den Fingerprint auf Android unabhängig vergleichen. Keine Router-Portfreigaben.')
            return
        if self.lan_host is None:
            candidates=set()
            try:
                for entry in socket.getaddrinfo(socket.gethostname(),None,family=socket.AF_INET):
                    address=entry[4][0]
                    ip=ipaddress.ip_address(address)
                    if ip.is_private and not ip.is_loopback and not ip.is_link_local:
                        candidates.add(address)
            except OSError: pass
            items=sorted(candidates)
            if not items:
                QMessageBox.warning(self,'LAN nicht gefunden',
                    'Keine private IPv4-Adresse erkannt. LAN-Verbindung am Mac prüfen.');return
            host,ok=QInputDialog.getItem(self,'Android-LAN-Zugang aktivieren',
                'Freizugebende lokale Mac-IP (nur deren /24-Netz):',items,0,False)
            if not ok or not host:return
            if QMessageBox.question(self,'Netzwerkfreigabe bestätigen',
                f'Den NVR ausschließlich an {host}:8443 für Geräte im lokalen /24-Netz freigeben? '
                'Router-Portfreigaben dürfen NICHT eingerichtet werden.')!=QMessageBox.Yes:return
            self.launch_https(host)
        if self.network_server is None:
            QMessageBox.warning(self,'HTTPS API',getattr(self,'network_error','Server konnte nicht gestartet werden'));return
        code=self.network_service.devices.create_pairing()
        QMessageBox.information(self,'Android sicher koppeln',
            'Lokaler NVR: https://'+self.lan_host+':8443\n\n'
            'Einmaliger Kopplungscode (5 Minuten): '+code+'\n\n'
            'SHA-256-Zertifikatsfingerprint:\n'+cert_fingerprint()+'\n\n'
            'Fingerprint auf Android vollständig vergleichen.\n'
            'VPN für Fernzugriff; niemals Router-Portfreigaben für NVR/Kameras.')

    def manage_devices(self):
        devices=self.network_service.devices
        if not devices.devices:
            QMessageBox.information(self,'Android-Geräte','Keine Geräte gekoppelt');return
        names=[f"{dev['name']} · {ident}" for ident,dev in devices.devices.items()]
        item,ok=QInputDialog.getItem(self,'Gerät entkoppeln','Autorisierte Geräte',names,0,False)
        if ok and item and QMessageBox.question(self,'Zugang entziehen',item+' wirklich sperren?')==QMessageBox.Yes:
            devices.revoke(item.rsplit(' · ',1)[1]);self.events.log('Android-Zugang entzogen')

    def refresh_tiles(self):
        for tile in self.tiles:tile.shutdown();tile.setParent(None);tile.deleteLater()
        self.tiles=[]
        while self.grid.count():
            item=self.grid.takeAt(0)
            if item.widget():item.widget().setParent(None)
        self.manager.sync_cameras();cams=[c for c in self.cfg['cameras'] if c.get('enabled',True)]
        self.counter.setText(f'{len(cams)} aktive Kamera(s)    /    {self.cfg["record_mode"].upper()}    /    Speicher: {self.cfg["record_dir"]}')
        for n,cam in enumerate(cams):
            tile=Tile(cam,self.manager,self.events,self.cfg);self.tiles.append(tile);self.grid.addWidget(tile,n//2,n%2)
        if not cams:
            placeholder=QLabel('NO SIGNAL  ·  Über ＋ Kamera einen RTSP- oder HTTP-Stream hinzufügen')
            placeholder.setAlignment(Qt.AlignCenter);placeholder.setStyleSheet('color:#829eb8;font-size:18px');self.grid.addWidget(placeholder,0,0)
        self.tick()
    def camera_select(self):
        if not self.cfg['cameras']:return None
        items=[x['name'] for x in self.cfg['cameras']]
        item,ok=QInputDialog.getItem(self,'Kamera wählen','Kamera',items,0,False)
        return next((c for c in self.cfg['cameras'] if c['name']==item),None) if ok else None
    def add_cam(self):
        dialog=AddCamera(parent=self)
        if dialog.exec()==QDialog.Accepted:
            self.cfg['cameras'].append({'id':secrets.token_hex(8),'name':dialog.name.text().strip(),'url':dialog.url.text().strip(),'enabled':dialog.enabled.isChecked()})
            save_config(self.cfg);self.events.log('Kamera hinzugefügt',dialog.name.text().strip());self.refresh_tiles()
    def edit_cam(self):
        cam=self.camera_select()
        if not cam:return
        dialog=AddCamera(cam,self)
        if dialog.exec()==QDialog.Accepted:
            cam.update(name=dialog.name.text().strip(),url=dialog.url.text().strip(),enabled=dialog.enabled.isChecked())
            save_config(self.cfg);self.events.log('Kamera bearbeitet',cam['name']);self.refresh_tiles()
    def remove_cam(self):
        cam=self.camera_select()
        if not cam:return
        if QMessageBox.question(self,'Kamera entfernen',f'Kamera „{cam["name"]}“ wirklich entfernen?')!=QMessageBox.Yes:return
        self.cfg['cameras'].remove(cam);save_config(self.cfg);self.events.log('Kamera entfernt',cam['name']);self.refresh_tiles()
    def settings(self):
        if Settings(self.cfg,self).exec()==QDialog.Accepted:
            self.events.log('Einstellungen geändert');self.refresh_tiles()
    def change_dir(self):
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') == '1':
            QMessageBox.warning(self,'Vault geschützt','Das Archivverzeichnis ist im Appliance-Modus fest an den LUKS-Vault gebunden.');return
        folder=QFileDialog.getExistingDirectory(self,'Aufnahmeverzeichnis wählen',self.cfg['record_dir'])
        if folder:self.cfg['record_dir']=folder;save_config(self.cfg);self.refresh_tiles();self.update_recordings()
    def open_folder(self,folder):
        ensure_private(Path(folder));QDesktopServices.openUrl(QUrl.fromLocalFile(str(Path(folder))))
    def update_recordings(self):
        self.paths=recording_files(self.cfg['record_dir']);self.files.setRowCount(len(self.paths))
        for row,path in enumerate(self.paths):
            stat=path.stat()
            for col,value in enumerate((path.name,f'{stat.st_size/1024/1024:.1f} MB',dt.datetime.fromtimestamp(stat.st_mtime).strftime('%d.%m.%Y %H:%M'))):
                self.files.setItem(row,col,QTableWidgetItem(value))
    def play_recording(self):
        row=self.files.currentRow()
        if row<0 or row>=len(self.paths):return
        PlaybackWindow(self.paths[row],self).exec()
    def update_events(self):
        rows=self.events.recent();self.event_table.setRowCount(len(rows))
        for i,r in enumerate(rows):
            for j,val in enumerate(r):self.event_table.setItem(i,j,QTableWidgetItem(str(val)))
    def tab_changed(self,index):
        if index==1:self.update_recordings()
        if index==2:self.update_events()
    def tick(self):
        if os.environ.get('DYNAMICOS_EXTERNAL_RECORDER') != '1':
            self.manager.tick()
        if os.environ.get('DYNAMICOS_APPLIANCE_MODE') != '1':
            self.network_service.relay.reap()
        self.clock.setText(dt.datetime.now().strftime('%d.%m.%Y · %H:%M:%S'))
        ongoing=sum(rec.running for rec in self.manager.recorders.values())
        if os.environ.get('DYNAMICOS_EXTERNAL_RECORDER') == '1':
            try:
                info=json.loads((Path(os.environ['DYNAMICOS_STATE_DIR']) / 'recorder-status.json').read_text())
                if time.time()-info.get('epoch',0)<30:ongoing=info.get('running',0)
            except (OSError,ValueError,KeyError):pass
        self.status.setText(f'● SYSTEM ONLINE     /     {ongoing} AKTIVE AUFZEICHNUNGEN     /     FFmpeg {"BEREIT" if ffmpeg_path() else "FEHLT"}')
    def closeEvent(self,event):
        self.timer.stop()
        for tile in self.tiles:tile.shutdown()
        self.network_service.relay.stop()
        if self.network_server is not None: self.network_server.should_exit=True
        self.manager.shutdown();self.events.log('Abmeldung',details='Desktop-Anwendung geschlossen')
        event.accept()


def main():
    app=QApplication(sys.argv);app.setApplicationName('dynamicOS NVR');app.setStyleSheet(STYLE)
    cfg=load_config()
    if Login(cfg).exec()!=QDialog.Accepted:return 0
    view=Main(cfg);view.show();return app.exec()

if __name__=='__main__':sys.exit(main())
