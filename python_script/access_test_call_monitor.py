#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
from pathlib import Path
import os
import sys
import time

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

import requests
from scipy.io import wavfile


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
        # default=80,
        type=int,
    )
    parser.add_argument(
        '--language',
        default='en-US',
        type=str,
    )
    parser.add_argument(
        '--scene_id',
        default='access_test_scene_id',
        type=str,
    )
    parser.add_argument(
        '--filename',
        default='data/audio/beep_mute.wav',
        type=str,
    )
    args = parser.parse_args()
    return args


def main():
    """长音频测试
    python3 access_test_call_monitor.py --host 127.0.0.1 --port 4070 --language en-US --scene_id 0649igfxch
    python3 access_test_call_monitor.py --host 127.0.0.1 --port 4070 --language zh-TW --scene_id 0649igfxch---
    """
    args = get_args()

    # http://10.20.251.14:4070/update_stream
    url = 'http://{host}:{port}/asrserver/update_stream'.format(
        host=args.host,
        port=args.port,
    )

    headers = {
        'Content-Type': 'application/json'
    }

    filename = os.path.join(project_path, args.filename)

    call_id = hashlib.md5(filename.encode(encoding='UTF-8')).hexdigest()
    call_id = '{}_{}'.format(call_id, time.time())

    # filename = args.filename
    sample_rate, signal = wavfile.read(filename)
    # signal = signal[:16000 * 3]
    if signal.ndim == 2:
        signal = signal[:, 0]

    win_size = 2.0
    win_step = 2.0
    length = len(signal)
    print('length: {}'.format(length / sample_rate))

    idx = 0
    while True:
        begin = idx * win_step * sample_rate
        end = begin + win_size * sample_rate
        begin = int(begin)
        end = int(end)
        if begin >= length:
            break
        if end > length:
            end = length

        sub_signal = signal[begin: end]
        if end - begin < 10:
            print('end - begin = {} < 10'.format(end - begin))
            break
        wavfile.write('temp.wav', sample_rate, sub_signal)

        # print(sample_rate)
        print(sub_signal.shape)

        with open('temp.wav', 'rb') as f:
            data = f.read()

        base64string = base64.b64encode(data).decode('utf-8')
        # print(base64string)

        data = {
            'language': args.language,
            'call_id': 'access_test_call_id_{}'.format(call_id),
            'scene_id': args.scene_id,
            'signal': base64string,
        }

        resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=2)

        if resp.status_code == 200:
            js = resp.json()
            print(js)
            # print(resp.text)
            result = js['result']
            # print(result)

            status = result['status']
            if status == 'finished':
                break
        else:
            print(resp.status_code)
            print(resp.text)

        # exit(0)
        idx += 1
    return


if __name__ == '__main__':
    main()
