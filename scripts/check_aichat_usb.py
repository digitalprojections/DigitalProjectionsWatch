"""Diagnose AI Chat with --connect; retain raw logs privately and print only state/memory."""
import argparse, json, re
from pathlib import Path
import serial
from watch_usb import collect
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True);p.add_argument('--connect',action='store_true');args=p.parse_args()
 if not args.connect:p.error('--connect is required: AI Chat sends existing identity/authentication to its configured provider')
 port=serial.Serial();port.port=args.port;port.baudrate=115200;port.timeout=.05;port.dtr=False;port.rts=False;port.open()
 raw=bytearray();states=[]
 def command(c,seconds):
  port.write((c+'\r\n').encode());data=collect(port,seconds);raw.extend(data);return re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',data.decode(errors='replace'))
 try:
  raw.extend(collect(port,1));apps=command('watch',1);ids={name:int(i) for i,name in re.findall(r'APP (\d+) ([^\r\n]+)',apps)}
  command('watch open '+str(ids['AIChats']),5)
  for _ in range(12):
   text=command('ai_chat',2);match=re.search(r'AI_CHAT state=(\d+) free_internal=(\d+) largest_internal=(\d+) free_psram=(\d+)',text)
   if match:
    state=dict(zip(('state','free_internal','largest_internal','free_psram'),map(int,match.groups())));states.append(state)
    if state['state'] in (2,4,5,6,7):break
  text=raw.decode(errors='replace')
  evidence={'states':states,'audio_ready':'XiaozhiAudio: Ready:' in text,'audio_allocation_failed':'Failed to create AFE processing task' in text,'crash':any(x in text for x in ('Guru Meditation','Backtrace:','stack overflow','abort()'))}
  print(json.dumps(evidence,indent=2))
 finally:
  port.close();d=ROOT/'.local';d.mkdir(exist_ok=True);(d/'aichat-usb.log').write_bytes(raw)
  if 'evidence' in locals():(d/'aichat-usb-evidence.json').write_text(json.dumps(evidence,indent=2))
if __name__=='__main__':main()
