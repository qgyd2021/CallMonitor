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
        default='unittest_cnn_voicemail_scene_id',
        type=str,
    )
    parser.add_argument(
        '--data_cnn_voicemail_dir',
        default='data/unittest_data/cnn_voicemail',
        type=str,
    )
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    data_cnn_voicemail_dir = project_path / args.data_cnn_voicemail_dir
    filename_list = data_cnn_voicemail_dir.glob('*/*/*.wav')

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
            'call_id': 'unittest_cnn_voicemail_call_id_{}'.format(call_id),
            'scene_id': args.scene_id,
            'signal': base64string,
        }

        resp = requests.post(url, headers=headers, data=json.dumps(data), timeout=None)
        if resp.status_code == 200:
            js = resp.json()
            tasks = js['result']['tasks']
            for task in tasks:
                task_name = task['task_name']
                label = task['label']
                if task_name == 'cnn voicemail':
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
            print('request failed, status_code: {}, text: {}'.format(resp.status_code, resp.text))
    return


if __name__ == '__main__':
    main()
