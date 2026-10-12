"""Verify internet time, timezone UI persistence and live status-bar bounds over USB."""
import argparse,json,re,time,datetime
from pathlib import Path
import serial
from watch_usb import collect
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--port',required=True);args=p.parse_args()
 raw=bytearray();evidence=[];port=serial.Serial();port.port=args.port;port.baudrate=115200;port.timeout=.05;port.dtr=False;port.rts=False;port.open()
 def cmd(text,seconds=.8):
  port.write((text+'\r\n').encode());data=collect(port,seconds);raw.extend(data);s=re.sub(r'\x1b\[[0-9;?]*[A-Za-z]','',data.decode(errors='replace'))
  if any(x in s for x in ('Guru Meditation','Backtrace:','stack overflow','abort()')):raise RuntimeError('Device crash')
  return s
 def clock():
  s=cmd('clock');lines=[l for l in s.splitlines() if l.startswith('CLOCK ')];assert lines,'No clock status';return dict(re.findall(r'(\w+)=([^ ]+)',lines[-1]))
 def bounds():
  s=cmd('clock ui');lines=[l for l in s.splitlines() if l.startswith('CLOCK_UI ')];assert lines,'No layout diagnostic';d=dict(re.findall(r'(\w+)=([^ ]+)',lines[-1]));assert d['visible']=='1' and d['safe']=='1',d;return d
 try:
  raw.extend(collect(port,1));apps=cmd('watch',1);ids={name:int(i) for i,name in re.findall(r'APP (\d+) ([^\r\n]+)',apps)}
  deadline=time.monotonic()+95
  while True:
   s=clock()
   if s['valid']=='1' and s['source']=='2' and s['rtc_saved']=='1':break
   if time.monotonic()>deadline:raise RuntimeError('Internet time or RTC save not confirmed: '+str(s))
  delta=abs(int(s['epoch'])-int(datetime.datetime.now(datetime.timezone.utc).timestamp()));assert delta<5,(delta,s)
  evidence.append({'internet_time':s,'host_utc_delta_seconds':delta})
  cmd('watch open '+str(ids['Settings']),2);cmd('clock page',1)
  cmd('clock zone 2',1);s=clock();assert s['zone']=='2' and s['local'].endswith('+0000'),s;evidence.append({'utc_zone_ui':s})
  cmd('clock zone 0',1);s=clock();assert s['zone']=='0' and s['local'].endswith('+0900'),s;evidence.append({'japan_zone_ui':s})
  evidence.append({'settings_top_bar':bounds()})
  for name in ('2048','Recorder','GravitySphere'):
   cmd('watch open '+str(ids[name]),2);evidence.append({'app':name,'top_bar':bounds()})
  cmd('watch open '+str(ids['Settings']),2);cmd('clock page',1)
  print(json.dumps({'result':'passed','checks':evidence},indent=2))
 finally:
  port.close();d=ROOT/'.local';d.mkdir(exist_ok=True);(d/'clock-usb.log').write_bytes(raw);(d/'clock-usb-evidence.json').write_text(json.dumps(evidence,indent=2))
if __name__=='__main__':main()
