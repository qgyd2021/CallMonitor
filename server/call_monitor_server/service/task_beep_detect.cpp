//
// Created by tianx on 2023/4/19.
//
#include <chrono>
#include <cstdint>
#include <map>
#include <string>

#include <glog/logging.h>

#include "report_asr_event.h"
#include "task.h"
#include "task_beep_detect.h"
#include "toolbox/utils/wav.h"
#include "../settings.h"


BeepDetectContextProcess::BeepDetectContextProcess(float win_len, float win_step, std::int64_t n_fft) :
                          TaskContextProcess(),
                          win_len_(win_len),
                          win_step_(win_step),
                          n_fft_(n_fft) {
  for (int i = 0; i <= max_length_; ++i) {
    score_queue_.push(0.);
  }
}


torch::Tensor BeepDetectContextProcess::calc_mag_spec(
    const torch::Tensor & signal, std::int64_t n_fft,
    std::int64_t hop_length, std::int64_t win_length)
{

  torch::Tensor window = torch::hann_window(win_length, torch::kFloat);

  //shape=[nFreq, timeSteps]
  torch::Tensor spectrum = torch::stft(
      signal,
      n_fft,
      hop_length,
      win_length,
      {window},
      false,
      true,
      true
  );

  torch::Tensor magspec = torch::abs(spectrum);
  return std::move(magspec);
}


bool BeepDetectContextProcess::update(std::string language, std::string call_id,
                                      std::string scene_id, const toolbox::WavFile & wav_file) {
  if (status_.compare("finished") != 0) {

    language_ = language;
    call_id_ = call_id;
    scene_id_ = scene_id;

    sample_rate_ = wav_file.sample_rate();
    signal_size_ = wav_file.num_samples();

    signal_.clear();
    float sample = 0.0;
    for (int i = 0; i < wav_file.num_samples(); ++i) {
      // make sample to [-1, 1]
      sample = (* (wav_file.data() + i)) / (1 << 15);
      signal_.push_back(sample);
    }
    duration_ += (float) wav_file.num_samples() / (float) wav_file.sample_rate();
    return true;
  } else {
    return false;
  }
}


bool BeepDetectContextProcess::decision()
{
  bool result = false;

  auto win_length = (std::int64_t) (win_len_ * sample_rate_);
  auto hop_length = (std::int64_t) (win_step_ * sample_rate_);
  //long long size = signal_size_;
  torch::Tensor signal_tensor = torch::from_blob(signal_.data(), {signal_size_}, torch::kFloat);

  //shape=[n_freq, time_steps]
  torch::Tensor magspec = this->calc_mag_spec(
      signal_tensor,
      n_fft_,
      hop_length,
      win_length
  );
  torch::Tensor magspec_t = magspec.t();

  float score = 0;
  float max_score = 0.;
  float total_score = 0.;
  float mean_score = 0.;

  for (int64_t i = 0; i < magspec_t.size(0); ++i) {
    torch::Tensor magspec_t_frame = magspec_t.index({i, "..."});

    torch::Tensor e0 = torch::sum(magspec_t_frame);
    max_score = 0.;

    for (int64_t j = freq_begin_; j < freq_end_; ++j) {
      torch::Tensor magspec_t_frame_block = magspec_t_frame.index(
          {torch::indexing::Slice(j, j + freq_win_size_)});
      torch::Tensor e1 = torch::sum(magspec_t_frame_block);
      torch::Tensor tensor_score = e1 / e0;
      score = * tensor_score.data_ptr<float>();
      if (score > max_score) {
        max_score = score;
      };
    };
    total_score += max_score;
    total_score -= score_queue_.front();
    score_queue_.pop();
    score_queue_.push(max_score);

    mean_score = total_score / this->max_length_;

    if (mean_score > score_threshold_) {
      result = true;
      break;
    };
  }
  return result;
}


void BeepDetectContextProcess::process(std::string language, std::string call_id,
                                       std::string scene_id, const toolbox::WavFile & wav_file) {
  bool update_flag = this->update(language, call_id, scene_id, wav_file);
  if ( ! update_flag ) {
    return;
  }

  bool decision_flag = this->decision();

  if (decision_flag) {
    std::string product_id = "callbot";
    //voicemail correspond to eventType 1
    int event_type = 1;
    std::string text = "voicemail";
    //std::string text = "beep";

    LOG(INFO) << "report voicemail; duration: " << duration_ \
              << ", language: " << language \
              << ", call_id: " << call_id \
              << ", scene_id: " << scene_id;

    bool report_flag = report_asr_event(
        product_id,
        call_id_,
        language_,
        event_type,
        text,
        FLAGS_asr_event_http_host_port,
        FLAGS_asr_event_uri,
        FLAGS_secret_key
    );
    label_ = "beep_detect";
    status_ = "finished";
  } else if (duration_ >= 20.) {
    //只检测前20秒.
    label_ = "unknown";
    status_ = "finished";
  } else {
    //
  }

}


BeepDetectContextProcess * BeepDetectManager::get_context(std::string call_id) {
  cache_update_lock_.lock();

  BeepDetectContextProcess * context;

  std::map<std::string, BeepDetectContextProcess * >::iterator item = context_process_cache_.find(call_id);
  if (item == context_process_cache_.end()) {
    context = new BeepDetectContextProcess(0.025, 0.01, 512);
    context_process_cache_[call_id] = context;
  } else {
    context = item->second;
    context->update_timestamp();
  }
  cache_update_lock_.unlock();
  return context;
}


void BeepDetectManager::clear_context() {
  auto second_clock = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  std::int64_t timestamp_now = second_clock.count();

  LOG(INFO) << "task beep detect, this->cache.size(): " << context_process_cache_.size();

  if (timestamp_now - cache_last_update_time_ > 6) {
    cache_last_update_time_ = timestamp_now;
    cache_update_lock_.lock();

    //search the context that timeout
    std::list<std::string> timeout_contexts;
    for (std::map<std::string, BeepDetectContextProcess *>::iterator it = context_process_cache_.begin();
         it != context_process_cache_.end(); ++it) {
      std::int64_t duration = timestamp_now - it->second->last_update_time_;

      //超过6秒没有更新过的上下文,清除.
      if (duration > 6) {
        timeout_contexts.push_back(it->first);
      }
    }

    //erase the timeout context
    for (std::list<std::string>::iterator it = timeout_contexts.begin(); it != timeout_contexts.end(); ++it) {
      std::map<std::string, BeepDetectContextProcess *>::iterator item = context_process_cache_.find(*it);
      if (item != context_process_cache_.end()) {
        delete item->second;
        context_process_cache_.erase(*it);
      }
    }
    cache_update_lock_.unlock();
  }
}


TaskContextProcess * BeepDetectManager::process(
    std::string language, std::string call_id, std::string scene_id, const toolbox::WavFile & wav_file) {
  BeepDetectContextProcess * context_ptr = this->get_context(call_id);
  context_ptr->process(language, call_id, scene_id, wav_file);
  this->clear_context();
  return context_ptr;
}
