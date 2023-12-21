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

import pandas as pd
import requests
from scipy.io import wavfile
from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log_file", default="CallMonitor.INFO", type=str)
    parser.add_argument("--language", default="vi-VN", type=str)

    args = parser.parse_args()
    return args


def main():
    args = get_args()

    with open(args.log_file, "r", encoding="utf-8") as f:
        for row in f:
            row = str(row).strip()
            # if not row.__contains__(args.language):
            #     continue

            match1 = re.search(r"request language: (.*?)response body: (.*?)", row, flags=re.I)
            if match1 is None:
                continue

            match2 = re.search(r"max_duration_threshold: (.*?)max_energy_threshold: (.*?)", row, flags=re.I)
            if match2 is None:
                continue
            print(match1.group(0))
            print(match2)
            print(match2.group(0))
            print(match2.group(1))
    return


if __name__ == '__main__':
    main()
