#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
from pathlib import Path
import os
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

from scipy.io import wavfile
import torch


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--wav_file',
        default='data/audio/beep_mute.wav',
        type=str,
    )
    parser.add_argument(
        '--model_file',
        default='trained_models/cnn_voicemail_tw_20230411/pth/cnn_voicemail.pth',
        type=str,
    )
    args = parser.parse_args()
    return args


def main():
    args = get_args()

    model = torch.jit.load(project_path / args.model_file)

    sample_rate, waveform = wavfile.read(project_path / args.wav_file)
    waveform = waveform[:16000]
    waveform = waveform / (1 << 15)
    inputs = torch.from_numpy(waveform)
    inputs = torch.unsqueeze(inputs, 0)
    inputs = inputs.float()
    outputs = model(inputs)
    print(outputs)
    return


if __name__ == '__main__':
    main()
