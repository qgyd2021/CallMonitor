#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
from pathlib import Path
import os
import shutil
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, "../"))
sys.path.append(project_path)

import requests
from tqdm import tqdm


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", default=5000, type=int)
    parser.add_argument("--src-dir", default="src/dir", type=str)
    parser.add_argument("--tgt-dir", default="tgt/dir", type=str)
    args = parser.parse_args()
    return args


x_access_token = """
eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE2OTgwNDg3MjIsImRhdGEiOnsiYWNjb3VudCI6Ik5YMDgzNzdAOWVWRjJsVTJKLnRpYm90LmNvbSIsImlkIjoiNTUwMyIsIm5hbWUiOiJOWDA4Mzc3LUFJYm905bmz5Y-wIiwicm9sZSI6MSwib3BlcmF0ZUdyYW50IjozMX0sImlhdCI6MTY5ODA0MTUyMn0.Uj0yHWV7YWEPeS9hmX10d1NAEUEiDsncMSI0BlCOk7U
"""


def get_info_by_call_id(call_id: str, key: str = "sceneId"):
    url = "https://nxbot.nxcloud.com/api/tasks"

    headers = {
        "X-Access-Token": x_access_token.strip(),
    }

    params = {
        "current": 1,
        "pageSize": 10,
        "taskStatus": -1,
        "sceneId": "",
        "startTime": "",
        "endTime": "",
        "taskIds": call_id,
        "subuser": "",
        "searchType": "call",
        "productId": "callbot",
    }

    resp = requests.request(
        method="GET",
        url=url,
        headers=headers,
        params=params,
    )

    if resp.status_code != 200:
        print(resp.status_code)
        print(resp.text)
        exit(0)

    js = resp.json()

    data = js["data"]["data"]

    if len(data) == 0:
        return None
    else:
        result = data[0][key]
        return result


def get_intent_id_by_call_id(call_id: str):
    task_id = get_info_by_call_id(call_id, key="taskId")

    url = "https://nxbot.nxcloud.com/api/call/info/byTask/{}".format(task_id)

    headers = {
        "X-Access-Token": x_access_token.strip(),
    }

    params = {
        "taskId": task_id,
        "searchIds": call_id,
        "searchType": "call",
        "current": 1,
        "pageSize": 10,
        "productId": "callbot",
        "callStatus[]": 6,
    }

    resp = requests.request(
        method="GET",
        url=url,
        headers=headers,
        params=params,
    )
    if resp.status_code != 200:
        print(resp.status_code)
        print(resp.text)
        exit(0)

    js = resp.json()

    data = js["data"]["data"]

    if len(data) == 0:
        return None
    else:
        result = data[0]["userIntentId"]
        return result


def get_intent_map_by_call_id(call_id: str):
    task_id = get_info_by_call_id(call_id, key="taskId")
    scene_id = get_info_by_call_id(call_id, key="sceneId")

    url = "https://nxbot.nxcloud.com/api/tasks/{}".format(task_id)

    headers = {
        "X-Access-Token": x_access_token.strip(),
    }

    params = {
        "sceneId": scene_id,
        "taskId": task_id,
        "productId": "callbot",

    }

    resp = requests.request(
        method="GET",
        url=url,
        headers=headers,
        params=params,
    )
    if resp.status_code != 200:
        print(resp.status_code)
        print(resp.text)
        exit(0)

    js = resp.json()
    # print(js)
    data = js["data"]

    if len(data) == 0:
        return None
    else:
        intent_tag = data["intentTag"]
        intent_id_to_intent_name = {
            it["intentId"]: it["intentDesc"] for it in intent_tag
        }
        return intent_id_to_intent_name


def main():
    args = get_args()
    src_dir = args.src_dir
    tgt_dir = args.tgt_dir
    src_dir = r"D:\programmer\asr_datasets\voicemail\origin_wav\ko-KR"
    tgt_dir = r"D:\programmer\asr_datasets\voicemail\origin_wav\language_temp"

    src_dir = Path(src_dir)
    tgt_dir = Path(tgt_dir)

    tgt_dir.mkdir(exist_ok=True)

    count = 0
    for filename in tqdm(src_dir.glob("*.wav")):
        if count < 4139:
            count += 1
            continue

        basename = filename.stem
        splits = basename.split("_")

        if len(splits) == 3:
            continue
            # call_id, language, time_stamp = splits
            # scene_id = get_info_by_call_id(call_id, key="sceneId")
            # scene_id = scene_id or "none"

        elif len(splits) == 4:
            call_id, language, scene_id, time_stamp = splits
        else:
            continue

        intent_id = get_intent_id_by_call_id(call_id)

        # voicemail
        intent_id_to_intent_name = get_intent_map_by_call_id(call_id)
        intent_name = intent_id_to_intent_name.get(intent_id, "default")
        if not intent_name.__contains__("语音信箱"):
            continue

        if count > args.limit:
            break

        # move
        to_filename = tgt_dir / "{}_{}_{}_{}.wav".format(call_id, language, scene_id, time_stamp)
        shutil.move(filename.as_posix(), to_filename.as_posix())

        count += 1

    return


if __name__ == "__main__":
    main()
