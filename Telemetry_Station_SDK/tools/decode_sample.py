#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Decode the included BEAR sequence/temperature sample, not arbitrary schemas."""
import argparse
import binascii
import json
import struct

SCHEMA_ID = 'ts-payload-v1-5a82b543'


def decode(text, payload_only=False):
    try:
        raw = bytes.fromhex(text)
    except ValueError as error:
        raise ValueError('Use hexadecimal bytes, such as 000000d7; spaces are allowed.') from error
    expected = 4 if payload_only else 6
    if len(raw) != expected:
        raise ValueError(f'Expected {expected} bytes for the included sample schema. Use --payload-only only when the transport CRC has already been removed.')
    if not payload_only and binascii.crc_hqx(raw[:4], 0) != int.from_bytes(raw[4:], 'big'):
        raise ValueError('Integrity check failed. Check the received bytes and schema; do not use this reading.')
    sequence, temperature = struct.unpack('>Hh', raw[:4])
    return {'schema_id': SCHEMA_ID, 'sequence': sequence, 'temperature_c': temperature / 10,
            'integrity': 'not_checked' if payload_only else 'crc_verified'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('hex_bytes', help='6 bytes: sample payload plus the 2-byte SDK transport CRC')
    parser.add_argument('--payload-only', action='store_true', help='decode 4 payload bytes without checking a transport CRC')
    args = parser.parse_args()
    try:
        result = decode(args.hex_bytes, args.payload_only)
    except ValueError as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
