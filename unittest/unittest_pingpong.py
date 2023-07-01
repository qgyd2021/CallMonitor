#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
from pathlib import Path
import os
import re
import sys
import time

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

import requests
from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='127.0.0.1', type=str)
    parser.add_argument('--port', default=4070, type=int)
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    # http://127.0.0.1:4070/asrserver/pingpong
    url = 'http://{host}:{port}/asrserver/pingpong'.format(
        host=args.host,
        port=args.port,
    )

    headers = {
        'Content-Type': 'application/json'
    }

    data = {
        'pingMs': int(time.time() * 1000),
        'pingMsg': 'pingpong',
    }

    expected_ping_msg = 'pingpong'
    resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=2)
    if resp.status_code == 200:
        js = resp.json()
        actually_ping_msg = js['pingMsg']
        if actually_ping_msg != expected_ping_msg:
            message = """unittest failed: 
                        expected_ping_msg: {expected_ping_msg};
                        actually_ping_msg: {actually_ping_msg};
                        """.format(expected_ping_msg=expected_ping_msg, actually_ping_msg=actually_ping_msg)
            message = re.sub(r'[\u0020]{5,}', '    ', message)
            print(message)
    else:
        print('request failed, status_code: {}'.format(resp.status_code))
    return


if __name__ == '__main__':
    main()
