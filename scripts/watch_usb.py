"""Run bounded USB console checks; retain raw logs in ignored local storage."""
import argparse
from pathlib import Path
import re
import time
import serial
ROOT = Path(__file__).resolve().parents[1]

def collect(port, duration):
    data = bytearray()
    until = time.monotonic() + duration
    while time.monotonic() < until:
        chunk = port.read(port.in_waiting or 1)
        data.extend(chunk)
        if b"\x1b[6n" in chunk:
            port.write(b"\x1b[1;1R")
    return bytes(data)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--command', action='append', default=[])
    parser.add_argument('--seconds', type=float, default=3)
    parser.add_argument('--log', default='usb-check.log')
    args=parser.parse_args()
    if not 0 < args.seconds <= 30: raise SystemExit('Seconds must be between 0 and 30.')
    port=serial.Serial()
    port.port=args.port;port.baudrate=115200;port.timeout=0.05
    port.dtr=False;port.rts=False;port.open()
    raw=collect(port, 1)
    try:
        for command in args.command:
            if '\r' in command or '\n' in command: raise SystemExit('One command per argument.')
            port.write((command+'\r\n').encode())
            raw+=collect(port,args.seconds)
        if not args.command: raw+=collect(port,args.seconds)
    finally: port.close()
    folder=ROOT/'.local';folder.mkdir(exist_ok=True)
    file=folder/Path(args.log).name;file.write_bytes(raw)
    clean=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '',raw.decode(errors='replace'))
    for line in clean.splitlines():
        if any(word in line for word in ['DISPLAY ', 'CLOCK ', 'CLOCK_UI ', 'DP:Time', 'RECORDER state=', 'PHONE_SETUP state=', 'MOTION state=', 'APP ', 'WATCH ', 'SD state=', 'SD_TEST ', 'Calibration requested', 'Guru Meditation', 'panic', 'abort()', 'Backtrace:', 'DP:Motion', 'Digital Projections Watch is ready', 'rst:']):
            print(line)
    print('Raw log saved privately:',file)

if __name__=='__main__': main()
