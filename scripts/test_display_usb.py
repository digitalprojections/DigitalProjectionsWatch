"""Verify actual AMOLED power commands and retained brightness over USB."""
import argparse,re,serial,json
from watch_usb import collect
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True);a=p.parse_args()
s=serial.Serial();s.port=a.port;s.baudrate=115200;s.timeout=.05;s.dtr=False;s.rts=False;s.open();raw=bytearray();records=[]
def command(text):
 s.write((text+'\r\n').encode());data=collect(s,.8);raw.extend(data);clean=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',data.decode(errors='replace'));m=re.search(r'DISPLAY state=(\w+) brightness=(\d+) consumes_boot=(\d+) pwr=\d+ error=(\w+)',clean);assert m,clean;return dict(zip(('state','brightness','consumes_boot','error'),m.groups()))
try:
 raw.extend(collect(s,1));s.write(b'watch\r\n');data=collect(s,1);raw.extend(data);apps=data.decode(errors='replace');m=re.search(r'APP (\d+) Settings',apps);assert m, 'Settings not found';s.write(('watch open '+m.group(1)+'\r\n').encode());raw.extend(collect(s,2));command('display page');collect(s,1);ui=command('display ui');assert ui['state']=='on';start=command('display on');assert start['state']=='on' and start['error']=='ESP_OK'
 for i in range(5):
  off=command('display sleep_button');assert off['state']=='off' and off['consumes_boot']=='1' and off['error']=='ESP_OK';assert off['brightness']==start['brightness']
  on=command('display on');assert on['state']=='on' and on['consumes_boot']=='0' and on['error']=='ESP_OK';assert on['brightness']==start['brightness'];records.append({'off':off,'on':on})
 print(json.dumps({'result':'passed','cycles':records},indent=2))
finally:
 try:command('display on')
 finally:
  s.close();from pathlib import Path;folder=Path(__file__).resolve().parents[1]/'.local';folder.mkdir(exist_ok=True);(folder/'display-usb.log').write_bytes(raw)
