#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
from pathlib import Path
import os
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

import requests


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--host',
        default='127.0.0.1',
        type=str,
    )
    parser.add_argument(
        '--port',
        default=4070,
        type=int,
    )
    args = parser.parse_args()
    return args


def main():
    # curl -X POST http://127.0.0.1:4070/HeartBeat -d '{"valStr": "tianxing", "valInt": 20221008}'
    args = get_args()

    url = 'http://{host}:{port}/HeartBeat'.format(
        host=args.host,
        port=args.port,
    )

    resp = requests.get(url)
    print(resp.text)
    return


if __name__ == '__main__':
    main()
