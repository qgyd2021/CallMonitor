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

import pandas as pd
import requests
from scipy.io import wavfile
from tqdm import tqdm


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
    parser.add_argument(
        '--scene_id',
        default='unittest_mute_detect_scene_id',
        type=str,
    )
    parser.add_argument(
        '--language',
        default='pt-BR',
        type=str,
    )
    parser.add_argument(
        '--data_beep_detect_dir',
        default=r'D:\Users\tianx\PycharmProjects\NlpBot\demo\音频格式转换\wav',
        type=str,
    )
    parser.add_argument(
        '--max_idx',
        default=5,
        type=int,
    )
    parser.add_argument(
        '--output_filename',
        default='result.xlsx',
        type=str,
    )
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    data_beep_detect_dir = project_path / args.data_beep_detect_dir
    filename_list = data_beep_detect_dir.glob('*.wav')

    result = list()
    for filename in tqdm(filename_list):

        sample_rate, signal = wavfile.read(filename)
        if signal.ndim == 2:
            signal = signal[:, 0]

        win_size = 2.0
        win_step = 2.0
        length = len(signal)

        last_resp = None
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

            idx += 1
            if idx > args.max_idx:
                break

            sub_signal = signal[begin: end]
            if end - begin < 10:
                print('end - begin = {} < 10'.format(end - begin))
                break
            wavfile.write('temp.wav', sample_rate, sub_signal)

            with open('temp.wav', 'rb') as f:
                data = f.read()

            base64string = base64.b64encode(data).decode('utf-8')

            call_id = filename.stem

            url = 'http://{host}:{port}/asrserver/update_stream'.format(
                host=args.host,
                port=args.port,
            )

            headers = {
                'Content-Type': 'application/json'
            }

            data = {
                'language': args.language,
                'call_id': 'unittest_mute_detect_call_id_{}'.format(call_id),
                'scene_id': args.scene_id,
                'signal': base64string,
            }

            last_resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=None)
        if last_resp.status_code == 200:
            js = last_resp.json()
            tasks = js['result']['tasks']
            for task in tasks:
                task_name = task['task_name']
                label = task['label']
                if task_name == 'mute detect':
                    result.append({
                        "call_id": filename.stem,
                        "language": args.language,
                        "filename": filename.as_posix(),
                        "label": label,
                        "message": task["message"]
                    })

    result = pd.DataFrame(result)
    result.to_excel(args.output_filename, index=False)
    return


if __name__ == '__main__':
    main()
