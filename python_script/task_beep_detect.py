#!/usr/bin/python3
# -*- coding: utf-8 -*-
import argparse
import base64
import hashlib
import json
import platform
from pathlib import Path
import os
import sys

pwd = os.path.abspath(os.path.dirname(__file__))
project_path = Path(os.path.join(pwd, '../'))
sys.path.append(project_path)

import matplotlib.pyplot as plt
import numpy as np
import torch
from scipy.io import wavfile


def main():
    """show spectrum"""
    MAX_WAV_VALUE = 32768.0

    filename = os.path.join(project_path, 'data/audio/beep_mute.wav')
    sample_rate, signal = wavfile.read(filename)
    signal = signal[:16000]
    # print(np.max(signal))
    signal = np.array(signal / (1 << 15), dtype=np.float32)
    # print(np.max(signal))

    win_len = 0.025
    win_step = 0.01
    n_fft = 512

    win_length = int(win_len * sample_rate)
    hop_length = int(win_step * sample_rate)

    freq_begin_ = 36
    freq_end_ = 54
    freq_win_size_ = 10
    max_length_ = 25
    score_threshold_ = 0.8

    tensor_signal = torch.from_numpy(signal).float()
    window = torch.hann_window(win_length, dtype=torch.float)

    spectrum = torch.stft(tensor_signal, n_fft, hop_length, win_length, window,
                          normalized=False, onesided=True, return_complex=True)

    magspec = spectrum.abs()
    magspec_t = magspec.T

    magspec_t_numpy = magspec_t.numpy()
    plt.gca().add_patch(
        plt.Rectangle(
            xy=(41, 1),
            width=8,
            height=38,
            fill=False,
            edgecolor='r',
            linewidth=1
        )
    )
    plt.imsave('beep.jpg', magspec_t_numpy)

    # if platform.system() == 'Windows':
    #     plt.imshow(magspec_t_numpy)
    #     plt.show()

    timesteps, n_freq = magspec_t.shape

    total_score = 0
    score_queue_ = [0] * max_length_
    for i in range(timesteps):
        magspec_t_frame = magspec_t[i]
        e0 = torch.sum(magspec_t_frame)
        max_score = 0

        for j in range(freq_begin_, freq_end_):
            magspec_t_frame_block = magspec_t_frame[j: j + freq_win_size_]
            e1 = torch.sum(magspec_t_frame_block)
            score = e1 / e0
            if score > max_score:
                max_score = score

        total_score += max_score
        total_score -= score_queue_[0]
        score_queue_.pop(0)
        score_queue_.append(max_score)
        mean_score = total_score / max_length_
        print(e0)

        if mean_score > score_threshold_:
            print('success')
            # break
    return


if __name__ == '__main__':
    main()
