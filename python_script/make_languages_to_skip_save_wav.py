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
    parser.add_argument('--wav_dir', default='D:/programmer/asr_datasets/voicemail/origin_wav', type=str)

    parser.add_argument('--limit', default=20000, type=int)
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    wav_dir = Path(args.wav_dir)
    languages = wav_dir.glob("*")
    for language in tqdm(languages):
        language = language.stem

        l_wav_dir = wav_dir / language
        for count, _ in enumerate(l_wav_dir.glob("*.wav")):
            if count > args.limit:
                print(language)
                break

    return


if __name__ == '__main__':
    main()
