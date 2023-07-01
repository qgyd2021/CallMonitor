#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import io
import os
from pathlib import Path
import pickle
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

from scipy.io import wavfile


def demo1():
    # filename = os.path.join(project_path, 'server/call_monitor/temp/update_stream_body_pkl/76a10e70-7876-4696-9dd2-c9fdb10583bc_1661647813.7404387.pkl')
    # filename = os.path.join(project_path, 'server/call_monitor/temp/update_stream_body_pkl/76a10e70-7876-4696-9dd2-c9fdb10583bc_1661647815.745367.pkl')
    filename = os.path.join(project_path, 'server/call_monitor/temp/update_stream_body_pkl/768d94bb-6814-46b5-b3db-9db9481b74e4_zh-CN_1665650431.907676.pkl')

    with open(filename, 'rb') as f:
        args: dict = pickle.load(f)
        print(args.keys())

        language = args['language']
        call_id = args['call_id']
        signal_base64string = args['signal_base64string']
        verbose = args['verbose']

        base64byte = signal_base64string.encode('utf-8')
        wav_bytes = base64.b64decode(base64byte)

        f = io.BytesIO(wav_bytes)
        sample_rate, signal = wavfile.read(f)

        wavfile.write('temp1.wav', sample_rate, signal)

    return


def demo2():
    """
    归类语音信箱识别收集的音频.
    """
    import shutil
    from glob import glob
    from tqdm import tqdm

    filename_pattern = r'D:\programmer\asr_datasets\voicemail\origin_wav\update_stream_body_pkl/*.pkl'
    filename_list = glob(filename_pattern)

    for filename in tqdm(filename_list):
        path, fn = os.path.split(filename)
        basename, ext = os.path.splitext(fn)

        with open(filename, 'rb') as f:
            args: dict = pickle.load(f)

            language = args['language']
            call_id = args['call_id']
            signal_base64string = args['signal_base64string']
            verbose = args['verbose']

            base64byte = signal_base64string.encode('utf-8')
            wav_bytes = base64.b64decode(base64byte)

            f = io.BytesIO(wav_bytes)
            sample_rate, signal = wavfile.read(f)

            to_path = os.path.join(path, '..', language)
            os.makedirs(to_path, exist_ok=True)
            to_filename = os.path.join(to_path, '{}.wav'.format(basename))
            wavfile.write(to_filename, sample_rate, signal)
        os.remove(filename)
    return


def demo3():
    import shutil
    from glob import glob
    from tqdm import tqdm

    filename_pattern = r'D:\programmer\asr_datasets\voicemail\zip_1\update_stream_wav/*.wav'
    filename_list = glob(filename_pattern)

    to_path = r'D:\programmer\asr_datasets\voicemail\origin_wav'

    for filename in tqdm(filename_list):
        path, fn = os.path.split(filename)
        basename, ext = os.path.splitext(fn)
        # print(basename)
        splits = basename.split('_')
        if len(splits) != 3:
            continue
        call_id, language, time_stamp = splits

        to_path_language = os.path.join(to_path, language)

        shutil.move(filename, to_path_language)
    return


def demo4():
    import zipfile
    import shutil
    from glob import glob
    from tqdm import tqdm

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


def demo5():
    """根据会话记录预分类音频"""
    from collections import defaultdict
    from glob import glob
    from tqdm import tqdm
    import pandas as pd
    import shutil

    filename_pattern = r'D:\程序员\ASR数据集\voicemail\origin_wav\th-TH\wav_unchecked_segmented\*.wav'
    filename_list = glob(filename_pattern)

    call_id_to_filename_list = defaultdict(list)
    for filename in tqdm(filename_list):
        path, fn = os.path.split(filename)
        basename, ext = os.path.splitext(fn)
        call_id, _ = basename.split('_th-TH_', maxsplit=1)
        call_id_to_filename_list[call_id].append(filename)

    df = pd.read_excel('tcje4kw6rk3k.xlsx')
    for i, row in tqdm(df.iterrows(), total=len(df)):
        call_id = row['call_id']
        user_intent = row['user_intent']

        if call_id in call_id_to_filename_list.keys():
            filename_list = call_id_to_filename_list[call_id]
            for filename in filename_list:
                path, _ = os.path.split(filename)
                to_path = os.path.join(path, user_intent)
                os.makedirs(to_path, exist_ok=True)

                shutil.move(filename, to_path)
            # exit(0)
    return


if __name__ == '__main__':
    # demo1()
    # demo2()
    # demo3()
    demo4()
