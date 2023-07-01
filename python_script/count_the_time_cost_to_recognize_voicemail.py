#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
from pathlib import Path
import os
import re
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

import pandas as pd


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--filename',
        default='logs/CallMonitor.INFO',
        # default='logs/CallMonitor.exe.LAPTOP-784USSUG.tianx.log.INFO.20230424-155258.17612',
        type=str)

    args = parser.parse_args()
    return args


def main():
    args = get_args()

    pattern = r'.*report voicemail; duration: (\d+), language: (\S+), call_id: (\S+), scene_id: (\S+)'

    result = list()
    count = 0
    with open(project_path / args.filename, 'r', encoding='utf-8') as f:
        for row in f:
            # row = 'I0609 08:21:31.152047 22445 task_cnn_voicemail.cpp:190] report voicemail; duration: 2, language: id-ID, call_id: 7aa7e6ff-523a-421f-87e8-fb9d1d1038d9, scene_id: bakw5yv06hl0'
            row = str(row).strip()
            if row.__contains__('report voicemail; duration: '):
                match = re.match(pattern, row, re.IGNORECASE)
                duration = match.group(1)
                language = match.group(2)
                call_id = match.group(3)
                scene_id = match.group(4)

                result.append({
                    'scene_id': scene_id,
                    'call_id': call_id,
                    'language': language,
                    'duration': int(duration),

                })
            if count % 1000 == 0:
                print('process, count: {}'.format(count))
            count += 1

    result = pd.DataFrame(result)
    result.to_excel('voicemail_time_cost.xlsx', index=False)
    return


if __name__ == '__main__':
    main()
