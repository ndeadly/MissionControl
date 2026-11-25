"""Native kiosk entrypoint; no shell, menu, browser or conventional desktop."""
import os,sys
os.environ.setdefault('QT_QPA_PLATFORM','xcb')
from nvr import main
if __name__=='__main__':sys.exit(main())
