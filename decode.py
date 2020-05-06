#!/usr/bin/env python3
import sys
import __future__

HEADER_BOOT =  0x00
HEADER_UPDATE = 0x01
HEADER_BUTTON_CLICK = 0x02
HEADER_BUTTON_HOLD  = 0x03

header_lut = {
    HEADER_BOOT: 'BOOT',
    HEADER_UPDATE: 'UPDATE',
    HEADER_BUTTON_CLICK: 'BUTTON_CLICK',
    HEADER_BUTTON_HOLD: 'BUTTON_HOLD'
}


def decode_temperature(buffer):
    if len(buffer) != 4:
        raise Exception("Bad temperature buffer len")

    if buffer == 'ffff':
        return None

    temperature = int(buffer, 16)

    if temperature:
        if temperature > 32768:
            temperature -= 65536
        temperature /= 10.0

    return temperature


def decode(data):
    if len(data) < 8:
        raise Exception("Bad data length, min 8 characters expected")

    header = int(data[0:2], 16)

    resp = {
        "header": header_lut[header],
        "voltage": int(data[2:4], 16) / 10.0 if data[2:4] != 'ff' else None,
    }

    for i, offset in enumerate(range(4, len(data), 4)):
        resp['temperature_%s' % i] = decode_temperature(data[offset:offset+4])

    return resp


def pprint(data):
    print('Header :', data['header'])
    print('Voltage :', data['voltage'])
    for i in range(10):
        key = 'temperature_%s' % i
        if key in data:
            print('Temperature %s:' % i, data[key])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        print("example: python3 decode.py 012000e500e7")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
