#!/usr/bin/python3
# -*- coding: utf-8 -*-
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
    filename_pattern1 = r'D:\programmer\asr_datasets\voicemail\*.zip'
    filename_list1 = glob(filename_pattern1)

    to_path = r'D:\programmer\asr_datasets\voicemail\origin_wav'

    for zip_filename in filename_list1:
        print(zip_filename)

        this_zip_file = zipfile.ZipFile(zip_filename)
        this_zip_file.extractall(path=r'D:\programmer\asr_datasets\voicemail')

        filename_pattern2 = r'D:\programmer\asr_datasets\voicemail\update_stream_wav/*.wav'
        filename_list2 = glob(filename_pattern2)

        for filename in tqdm(filename_list2):
            path, fn = os.path.split(filename)
            basename, ext = os.path.splitext(fn)
            # print(basename)
            splits = basename.split('_')
            if len(splits) != 3:
                continue
            call_id, language, time_stamp = splits

            to_path_language = os.path.join(to_path, language)
            os.makedirs(to_path_language, exist_ok=True)

            shutil.move(filename, to_path_language)

        this_zip_file.close()
        os.remove(zip_filename)
    return


if __name__ == '__main__':
    main()
