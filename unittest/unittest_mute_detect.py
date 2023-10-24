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
        '--data_beep_detect_dir',
        default='data/unittest_data/mute_detect',
        type=str,
    )
    parser.add_argument(
        '--max_duration_threshold',
        default=6.0,
        type=float,
    )
    parser.add_argument(
        '--max_idx',
        default=5,
        type=int,
    )
    args = parser.parse_args()
    return args


def main():
    print('Mute Detect: is a rule-based algorithm. \n')
    args = get_args()

    data_beep_detect_dir = project_path / args.data_beep_detect_dir
    filename_list = data_beep_detect_dir.glob('*/*/*.wav')

    for filename in filename_list:
        expected_label = filename.parts[-2]
        language = filename.parts[-3]

        sample_rate, signal = wavfile.read(filename)
        if signal.ndim == 2:
            signal = signal[:, 0]

        win_size = 2.0
        win_step = 2.0
        length = len(signal)
        # print('filename: {}\nlength: {}'.format(filename.as_posix(), length / sample_rate))

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

            # call_id = hashlib.md5(str(filename.stem).encode(encoding='UTF-8')).hexdigest()
            call_id = filename.stem

            # http://127.0.0.1:4070/asrserver/update_stream
            url = 'http://{host}:{port}/asrserver/update_stream'.format(
                host=args.host,
                port=args.port,
            )

            headers = {
                'Content-Type': 'application/json'
            }

            data = {
                'language': language,
                'call_id': 'unittest_mute_detect_call_id_{}'.format(call_id),
                'scene_id': args.scene_id,
                'signal': base64string,
            }

            last_resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=2)
        if last_resp.status_code == 200:
            js = last_resp.json()
            tasks = js['result']['tasks']
            for task in tasks:
                task_name = task['task_name']
                label = task['label']
                if task_name == 'mute detect':
                    if label != expected_label:
                        message = """unittest failed: 
                        filename: {filename}
                        language: {language}; expected_label: {expected_label};
                        message: {message};
                        actually_label: {actually_label};
                        """.format(filename=filename.as_posix(), language=language,
                                   expected_label=expected_label, message=task["message"],
                                   actually_label=label)
                        message = re.sub(r'[\u0020]{5,}', '    ', message)
                        print(message)
                    else:
                        message = """unittest success: 
                        filename: {filename}
                        language: {language}; expected_label: {expected_label};
                        message: {message};
                        actually_label: {actually_label};
                        """.format(filename=filename.as_posix(), language=language,
                                   expected_label=expected_label, message=task["message"],
                                   actually_label=label)
                        message = re.sub(r'[\u0020]{5,}', '    ', message)
                        print(message)
        else:
            print('request failed, status_code: {}'.format(last_resp.status_code))
    return


if __name__ == '__main__':
    main()
