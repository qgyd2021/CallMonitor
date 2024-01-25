#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import zipfile

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, "../"))
sys.path.append(project_path)

import numpy as np
import requests
from scipy.io import wavfile
from tqdm import tqdm
import torch


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", default=5000, type=int)
    parser.add_argument("--src-dir", default="src/dir", type=str)
    parser.add_argument("--tgt-dir", default="tgt/dir", type=str)
    parser.add_argument(
        "--model_file",
        default=(project_path / "trained_models/cnn_voicemail_common_20231130.zip").as_posix(),
        type=str
    )
    parser.add_argument("--label", default="voice", type=str)

    args = parser.parse_args()
    return args


def load_model(zip_file: Path):
    stem = zip_file.stem
    with zipfile.ZipFile(zip_file, "r") as f_zip:
        out_root = Path(tempfile.gettempdir()) / "cnn_voicemail"
        out_root.mkdir(parents=True, exist_ok=True)
        tgt_path = out_root / stem
        f_zip.extractall(path=out_root)

    model = torch.jit.load((tgt_path / "pth/cnn_voicemail.pth").as_posix())
    with open((tgt_path / "pth/labels.json").as_posix(), "r", encoding="utf-8") as f:
        labels = json.load(f)

    shutil.rmtree(tgt_path)

    d = {
        "model": model,
        "labels": labels,
    }
    return d


def predict(signal: np.ndarray, model_dict: dict) -> str:

    model = model_dict["model"]
    labels = model_dict["labels"]

    inputs = torch.tensor(signal, dtype=torch.float32)
    inputs = torch.unsqueeze(inputs, dim=0)

    outputs = model(inputs)

    probs = outputs["probs"]
    argmax = torch.argmax(probs, dim=-1)
    probs = probs.tolist()[0]
    argmax = argmax.tolist()[0]

    label = labels[argmax]
    prob = probs[argmax]

    return label, round(prob, 4)


def main():
    args = get_args()
    src_dir = args.src_dir
    tgt_dir = args.tgt_dir
    src_dir = r"D:\programmer\asr_datasets\voicemail\origin_wav\pt-BR"
    tgt_dir = r"D:\programmer\asr_datasets\voicemail\origin_wav\language_temp"

    src_dir = Path(src_dir)
    tgt_dir = Path(tgt_dir)

    tgt_dir.mkdir(exist_ok=True)

    # model
    d = load_model(Path(args.model_file))

    count = 0
    for filename in tqdm(src_dir.glob("*.wav")):

        if count < 0:
            count += 1
            continue

        basename = filename.stem
        splits = basename.split("_")

        if len(splits) == 3:
            # continue
            call_id, language, time_stamp = splits
            # scene_id = get_info_by_call_id(call_id, key="sceneId")
            # scene_id = scene_id or "none"
            scene_id = "none"

        elif len(splits) == 4:
            call_id, language, scene_id, time_stamp = splits
        else:
            continue

        # sample_rate, signal = wavfile.read(filename)
        # label, _ = predict(signal=signal, model_dict=d)
        # if label != args.label:
        #     continue

        if count > args.limit:
            break

        # move
        to_filename = tgt_dir / "{}_{}_{}_{}.wav".format(call_id, language, scene_id, time_stamp)
        shutil.move(filename.as_posix(), to_filename.as_posix())

        count += 1

    return


if __name__ == "__main__":
    main()
