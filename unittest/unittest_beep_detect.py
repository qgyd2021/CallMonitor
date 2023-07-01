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
        default='unittest_beep_detect_scene_id',
        type=str,
    )
    parser.add_argument(
        '--data_beep_detect_dir',
        default='unittest/data/beep_detect',
        type=str,
    )
    args = parser.parse_args()
    return args


def main():
    print('Beep Detect: is a rule-based algorithm, there may be many detections that are unsuccessful. \n'
          'It is recommend to train a deep learning model. ')
    args = get_args()

    data_beep_detect_dir = project_path / args.data_beep_detect_dir
    filename_list = data_beep_detect_dir.glob('*/*/*.wav')

    for filename in tqdm(filename_list):
        expected_label = filename.parts[-2]
        language = filename.parts[-3]

        with open(filename, 'rb') as f:
            data = f.read()

        base64string = base64.b64encode(data).decode('utf-8')

        call_id = hashlib.md5(str(filename.stem).encode(encoding='UTF-8')).hexdigest()
        call_id = '{}_{}'.format(call_id, time.time())

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
            'call_id': 'unittest_beep_detect_call_id_{}'.format(call_id),
            'scene_id': args.scene_id,
            'signal': base64string,
        }

        resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=2)
        if resp.status_code == 200:
            js = resp.json()
            tasks = js['result']['tasks']
            for task in tasks:
                task_name = task['task_name']
                label = task['label']
                if task_name == 'beep detect':
                    if label != expected_label:
                        message = """unittest failed: 
                        filename: {filename}
                        language: {language}; expected_label: {expected_label};
                        actually_label: {actually_label};
                        """.format(filename=filename.as_posix(), language=language,
                                   expected_label=expected_label, actually_label=label)
                        message = re.sub(r'[\u0020]{5,}', '    ', message)
                        print(message)
        else:
            print('request failed, status_code: {}'.format(resp.status_code))
    return


if __name__ == '__main__':
    main()
