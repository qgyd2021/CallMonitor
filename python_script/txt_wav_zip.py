#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
cp files in docker to host machine.
docker cp bdf6e2c7b91b:/data/tianxing/update_stream_wav/ /data/tianxing/update_stream_wav/

ls ./update_stream_wav | wc -c

"""
import argparse
from glob import glob
import os
import zipfile

from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--update_stream_wav_dir', default='./update_stream_wav', type=str)
    parser.add_argument('--pattern', default='*.txt', type=str)
    parser.add_argument('--limit', default=10000, type=int)
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    filename_pattern = os.path.join(args.update_stream_wav_dir, args.pattern)
    filename_list = glob(filename_pattern)

    zip_file: zipfile.ZipFile = None
    for idx, filename in tqdm(enumerate(filename_list), total=len(filename_list)):
        if zip_file is None:
            zip_file = zipfile.ZipFile('zip_{}.zip'.format(idx), 'w', zipfile.ZIP_DEFLATED)

        zip_file.write(filename)
        os.remove(filename)

        if idx % args.limit == 0:
            zip_file.close()
            zip_file = None

    return


if __name__ == '__main__':
    main()
