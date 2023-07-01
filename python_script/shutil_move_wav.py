#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
from pathlib import Path
import os
import shutil
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--limit', default=20000, type=int)
    parser.add_argument('--src-dir', default='src/dir', type=str)
    parser.add_argument('--tgt-dir', default='tgt/dir', type=str)
    args = parser.parse_args()
    return args


def main():
    args = get_args()
    src_dir = args.src_dir
    tgt_dir = args.tgt_dir
    src_dir = r'D:\programmer\asr_datasets\voicemail\origin_wav\en-US'
    tgt_dir = r'D:\programmer\asr_datasets\voicemail\origin_wav\language_temp'

    src_dir = Path(src_dir)
    tgt_dir = Path(tgt_dir)

    filename_list = src_dir.glob('*.wav')

    count = 0
    for filename in tqdm(filename_list):
        if count > args.limit:
            break
        shutil.move(str(filename), tgt_dir)
        count += 1

    return


if __name__ == '__main__':
    main()
