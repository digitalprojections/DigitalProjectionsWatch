import subprocess
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
VENDOR=ROOT/'examples/arduino/libraries'
BUILD=ROOT/'.build/preview'
subprocess.run(['cmake','-S',str(ROOT/'tests/watch/preview'),'-B',str(BUILD),f'-DVENDOR={VENDOR.as_posix()}','-G','Visual Studio 17 2022','-A','x64'],check=True)
subprocess.run(['cmake','--build',str(BUILD),'--config','Release','--parallel','4'],check=True)
subprocess.run([str(BUILD/'Release/preview.exe'),str(BUILD/'preview.ppm')],check=True)
Image.open(BUILD/'preview.ppm').save(ROOT/'.build/preview.png')
print(ROOT/'.build/preview.png')
