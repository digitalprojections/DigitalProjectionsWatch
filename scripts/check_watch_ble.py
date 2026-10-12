"""Read-only Bluetooth discovery and provisioning protocol check; no credentials sent."""
import argparse
import asyncio
import json
from bleak import BleakScanner, BleakClient

async def check(name):
    device = await BleakScanner.find_device_by_filter(
        lambda d, a: (a.local_name == name if name else (a.local_name or '').startswith('PROV_DP_')),
        timeout=15,
    )
    if device is None:
        raise SystemExit('Watch provisioning advertisement not found within 15 seconds.')
    print('WATCH_BLE advertisement found')
    async with BleakClient(device, timeout=15) as client:
        endpoints = {}
        for service in client.services:
            for characteristic in service.characteristics:
                for descriptor in characteristic.descriptors:
                    if descriptor.uuid.lower() == '00002901-0000-1000-8000-00805f9b34fb':
                        label = (await client.read_gatt_descriptor(descriptor.handle)).decode().rstrip('\0')
                        if label in ('proto-ver', 'prov-session', 'prov-config', 'prov-scan'):
                            endpoints[label] = characteristic
        required = {'proto-ver', 'prov-session', 'prov-config', 'prov-scan'}
        if not required.issubset(endpoints):
            raise SystemExit('Missing expected provisioning endpoints.')
        print('WATCH_BLE endpoints:', ', '.join(sorted(endpoints)))
        await client.write_gatt_char(endpoints['proto-ver'], b'---', response=True)
        payload = json.loads(bytes(await client.read_gatt_char(endpoints['proto-ver'])).decode())
        print('WATCH_BLE protocol:', json.dumps(payload))
        if payload['prov'].get('sec_ver') != 1:
            raise SystemExit('Expected encrypted security version 1.')
        print('WATCH_BLE read-only protocol check passed; Wi-Fi credentials unchanged.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', help='Exact advertised name when more than one watch is nearby')
    args = parser.parse_args()
    asyncio.run(check(args.name))
