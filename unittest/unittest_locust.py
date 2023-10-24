#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
from pathlib import Path
import os
import random
import re
import sys
import time

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

from locust import HttpUser, TaskSet, task
from locust.user.users import User
import requests
from tqdm import tqdm


class CallMonitorTaskSet(TaskSet):

    headers = {
        'Content-Type': 'application/json'
    }

    def __init__(self, parent: User) -> None:
        super().__init__(parent)
        self.filename_list = list()

    def refresh_filename_list(self):
        data_cnn_voicemail_dir = project_path / 'data/data/cnn_voicemail'

        data_cnn_voicemail_dir = project_path / data_cnn_voicemail_dir
        filename_list = data_cnn_voicemail_dir.glob('*/*/*.wav')
        filename_list = list(filename_list)
        # random.shuffle(filename_list)
        self.filename_list = filename_list

    @task
    def cnn_voicemail(self):
        scene_id = 'unittest_cnn_voicemail_scene_id'
        data_cnn_voicemail_dir = project_path / 'unittest/unittest_data/cnn_voicemail'

        data_cnn_voicemail_dir = project_path / data_cnn_voicemail_dir
        filename_list = data_cnn_voicemail_dir.glob('*/*/*.wav')
        filename_list = list(filename_list)

        random.shuffle(filename_list)

        if len(self.filename_list) == 0:
            self.refresh_filename_list()

        filename = self.filename_list.pop()
        language = filename.parts[-3]
        with open(filename, 'rb') as f:
            data = f.read()

        base64string = base64.b64encode(data).decode('utf-8')

        call_id = hashlib.md5(str(filename.stem).encode(encoding='UTF-8')).hexdigest()
        call_id = '{}_{}'.format(call_id, time.time())

        # http://127.0.0.1:4070/asrserver/update_stream
        url = '/asrserver/update_stream'

        data = {
            'language': language,
            'call_id': 'unittest_cnn_voicemail_call_id_{}'.format(call_id),
            'scene_id': scene_id,
            'signal': base64string,
        }
        resp = self.client.post(url, headers=self.headers, data=json.dumps(data))
        return


class RunTask(HttpUser):
    """
    http://docs.locust.io/en/stable/quickstart.html#direct-command-line-usage-headless

    # 10 users, access 1 times each second.
    locust -f unittest_locust.py --headless --users 10 --spawn-rate 1 -H http://127.0.0.1:4070

    # 20 users, access 5 times each second.
    locust -f unittest_locust.py --headless --users 10 --spawn-rate 5 -H http://127.0.0.1:4070
    """
    tasks = [CallMonitorTaskSet]


if __name__ == '__main__':
    # cmd = 'locust -f unittest_locust.py --headless --users 10 --spawn-rate 5 -H http://127.0.0.1:4070'
    # os.system(cmd)
    pass
