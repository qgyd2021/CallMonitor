#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
from glob import glob
import os
from pathlib import Path
import shutil
import sys
import zipfile

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zip_dir', default='D:/programmer/asr_datasets/voicemail', type=str)
    parser.add_argument('--to_path', default='D:/programmer/asr_datasets/voicemail/origin_wav', type=str)
    parser.add_argument('--zip_pattern', default='*.zip', type=str)
    parser.add_argument('--wav_pattern', default='*.txt', type=str)

    args = parser.parse_args()
    return args


def main():
    args = get_args()
    zip_dir = Path(args.zip_dir)
    filename_list1 = zip_dir.glob(args.zip_pattern)

    for zip_filename in filename_list1:
        print(zip_filename)

        this_zip_file = zipfile.ZipFile(zip_filename)
        this_zip_file.extractall(path=args.zip_dir)

        update_stream_wav = zip_dir / 'update_stream_wav'
        filename_list2 = update_stream_wav.glob(args.wav_pattern)
        for filename in tqdm(filename_list2):
            path, fn = os.path.split(filename)
            basename, ext = os.path.splitext(fn)
            splits = basename.split('_')
            if len(splits) != 4:
                continue
            call_id, language, scene_id, time_stamp = splits

            if language in ("pt-BR",):
                os.remove(filename)
                continue

            to_path_language = os.path.join(args.to_path, language)
            os.makedirs(to_path_language, exist_ok=True)

            # base64string to wav
            with open(filename, 'r', encoding='utf-8') as f:
                base64string = f.read()

            wav_bytes = base64.b64decode(base64string.encode('utf-8'))
            wav_filename = os.path.join(path, '{}.wav'.format(basename))
            with open(wav_filename, 'wb') as f:
                f.write(wav_bytes)
            os.remove(filename)
            shutil.move(wav_filename, to_path_language)

        this_zip_file.close()
        os.remove(zip_filename)

    return


if __name__ == '__main__':
    main()
