#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
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


def main():
    filename_pattern = './update_stream_wav/*.wav'
    filename_list = glob(filename_pattern)

    number = 10000
    zip_file: zipfile.ZipFile = None
    for idx, filename in tqdm(enumerate(filename_list), total=len(filename_list)):
        if zip_file is None:
            zip_file = zipfile.ZipFile('zip_{}.zip'.format(idx), 'w', zipfile.ZIP_DEFLATED)

        if str(filename).__contains__('id-ID'):
            continue
        if str(filename).__contains__('en-PH'):
            continue
        zip_file.write(filename)
        os.remove(filename)

        if idx % number == 0:
            zip_file.close()
            zip_file = None

    return


if __name__ == '__main__':
    main()
