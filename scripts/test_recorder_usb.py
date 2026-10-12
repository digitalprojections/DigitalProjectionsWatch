"""Non-destructive recorder playback journey through the production LVGL callbacks."""
import argparse
import json
import re
import time
from pathlib import Path
import serial
from watch_usb import collect
ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True)
    parser.add_argument('--completion-timeout',type=float,default=60,help='Seconds allowed for the selected recording to finish')
    args=parser.parse_args()
    raw=bytearray(); evidence=[]
    port=serial.Serial();port.port=args.port;port.baudrate=115200;port.timeout=.05;port.dtr=False;port.rts=False;port.open()
    def command(text,seconds=.6):
        port.write((text+'\r\n').encode()); data=collect(port,seconds);raw.extend(data)
        clean=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',data.decode(errors='replace'))
        if any(x in clean for x in ('Guru Meditation','abort()','Backtrace:','stack overflow')):raise RuntimeError('Device crash detected')
        return clean
    def status():
        text=command('recorder status')
        found=re.findall(r'RECORDER state=(\d+) stopped=(\d+) entries=(\d+) bytes=(\d+) total=(\d+) error=(\S+) owner=(\S+) applied=(\d+)',text)
        if not found:raise RuntimeError('Missing recorder status')
        values=found[-1];return dict(zip(('state','stopped','entries','bytes','total','error','owner','applied'),values))
    def wait_stopped(timeout=10):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            s=status()
            if s['stopped']=='1':return s
        raise RuntimeError('Recorder worker did not stop')
    try:
        raw.extend(collect(port,1))
        apps=command('watch',1)
        match=re.search(r'APP (\d+) Recorder\b',apps)
        if not match:raise RuntimeError('Recorder app not found')
        app_id=match.group(1)
        command('watch open '+app_id,2)
        command('recorder library')
        s=wait_stopped();assert s['error']=='ESP_OK' and int(s['entries'])>0,s
        evidence.append({'library':s})
        for cycle in range(3):
            command('recorder play1',1)
            s=status();assert int(s['bytes'])>0 and s['error']=='ESP_OK',s
            evidence.append({'play_cycle':cycle,'status':s})
            command('recorder stop')
            s=wait_stopped();assert s['owner']=='None' and s['error']=='ESP_OK',s
            evidence.append({'stop_cycle':cycle,'status':s})
        command('recorder play1',1)
        s=wait_stopped(args.completion_timeout);assert s['error']=='ESP_OK' and s['bytes']==s['total'] and s['owner']=='None',s
        evidence.append({'natural_end':s})
        command('recorder play1',1)
        command('watch home',2)
        command('watch open '+app_id,2)
        s=status();assert s['stopped']=='1' and s['owner']=='None',s
        evidence.append({'exit_during_playback':s})
        command('recorder library');s=wait_stopped();assert int(s['entries'])>0,s
        evidence.append({'reopen':s})
        print(json.dumps({'result':'passed','checks':evidence},indent=2))
    finally:
        port.close();folder=ROOT/'.local';folder.mkdir(exist_ok=True)
        (folder/'recorder-usb-journey.log').write_bytes(raw)
        (folder/'recorder-usb-evidence.json').write_text(json.dumps(evidence,indent=2))

if __name__=='__main__':main()
