#ifndef WAV_H_
#define WAV_H_
#include <cstdint>
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>


namespace toolbox {

struct WaveHeader{
  /*
  https://blog.csdn.net/weixin_48680270/article/details/123523517
  */

  //RIFF chunk descriptor
  char chunk_id[4] = {'R', 'I', 'F', 'F'};
  unsigned int chunk_size = 0;
  char wave[4] = {'W', 'A', 'V', 'E'};
  //fmt sub-chunk
  char fmt[4] = {'f', 'm', 't', ' '};
  unsigned int fmt_size = 16;
  uint16_t format = 1;
  uint16_t channels = 0;
  unsigned int sample_rate = 0;
  unsigned int bytes_per_second = 0;
  uint16_t block_size = 0;
  uint16_t bit = 0;
  char data[4] = {'d', 'a', 't', 'a'};
  unsigned int data_size = 0;
};

class WavFile {
public:
  WavFile() : data_(nullptr) {}
  explicit WavFile(const std::string& filename) { read(filename); }

  bool read(const std::string & filename) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (fp == NULL) {
      std::cout << "Error in read" << filename;
      return false;
    }

    WaveHeader header;
    fread(&header, 1, sizeof(header), fp);
    if (header.fmt_size < 16) {
      fprintf(stderr,
              "WaveData: expect PCM format data "
              "to have fmt chunk of at least size 16.\n");
      return false;
    } else if (header.fmt_size > 16) {
      int offset = 44 - 8 + header.fmt_size - 16;
      fseek(fp, offset, SEEK_SET);
      fread(header.data, 8, sizeof(char), fp);
    }

    num_channel_ = header.channels;
    sample_rate_ = header.sample_rate;
    bits_per_sample_ = header.bit;
    int num_data = header.data_size / (bits_per_sample_ / 8);
    num_samples_ = num_data / num_channel_;

    if (this->data_ == nullptr) {
      data_ = new float[num_data];
    } else {
      fprintf(stderr, "read file repeatedly. \n");
      exit(1);
    }

    for (int i = 0; i < num_data; ++i) {
      switch (bits_per_sample_) {
        case 8: {
          char sample;
          fread(&sample, 1, sizeof(char), fp);
          data_[i] = static_cast<float>(sample);
          break;
        }
        case 16: {
          int16_t sample;
          fread(&sample, 1, sizeof(int16_t), fp);
          data_[i] = static_cast<float>(sample);
          break;
        }
        case 32: {
          int sample;
          fread(&sample, 1, sizeof(int), fp);
          data_[i] = static_cast<float>(sample);
          break;
        }
        default:
          fprintf(stderr, "unsupported quantization bits. \n");
          exit(1);
      }
    }
    fclose(fp);
    return true;
  }

  bool fromBytes(unsigned char const* wav_bytes, unsigned long long int in_len) {
    WaveHeader header;
    memcpy(&header, wav_bytes, sizeof(header));
    if (header.fmt_size != 16) {
      fprintf(stderr,
              "WaveData: expect PCM format data "
              "to have fmt chunk of at least size 16.\n");
      return false;
    }

    num_channel_ = header.channels;
    sample_rate_ = header.sample_rate;
    bits_per_sample_ = header.bit;
    int num_data = header.data_size / (bits_per_sample_ / 8);
    num_samples_ = num_data / num_channel_;

    if (this->data_ == nullptr) {
      data_ = new float[num_data];
    } else {
      fprintf(stderr, "read file repeatedly. \n");
      return false;
    }

    unsigned char const* wav_data_bytes = wav_bytes + sizeof(header) / sizeof(unsigned char);
    for (int i = 0; i < num_data; ++i) {
      switch (bits_per_sample_) {
        case 8: {
          char sample;
          memcpy(&sample, wav_data_bytes, 1);
          wav_data_bytes += 1;
          data_[i] = static_cast<float>(sample);
          break;
        }
        case 16: {
          int16_t sample;
          memcpy(&sample, wav_data_bytes, 2);
          wav_data_bytes += 2;
          data_[i] = static_cast<float>(sample);
          break;
        }
        case 32: {
          int sample;
          memcpy(&sample, wav_data_bytes, 1);
          wav_data_bytes += 4;
          data_[i] = static_cast<float>(sample);
          break;
        }
        default:
          fprintf(stderr, "unsupported quantization bits. \n");
          exit(1);
      }
    }
    return true;
  }

  int num_channel() const { return num_channel_; }
  int sample_rate() const { return sample_rate_; }
  int bits_per_sample() const { return bits_per_sample_; }
  int num_samples() const { return num_samples_; }

  ~WavFile() {
    delete[] data_;
  }

  const float* data() const { return data_; }

private:
  int num_channel_;
  int sample_rate_;
  int bits_per_sample_;
  int num_samples_;  // sample points per channel
  float* data_ = nullptr;
};

} // namespace toolbox

#endif  // WAV_H_
