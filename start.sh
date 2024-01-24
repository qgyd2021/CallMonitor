#!/usr/bin/env bash

# sh start.sh --build_dir build --environment hk --http_port 4071
# sh start.sh --build_dir build --environment mx --http_port 4071


# dev, hk, gz
environment=dev
http_port=4070
build_dir="build"

# parse options
while true; do
  [ -z "${1:-}" ] && break;  # break if there are no arguments
  case "$1" in
    --*) name=$(echo "$1" | sed s/^--// | sed s/-/_/g);
      eval '[ -z "${'"$name"'+xxx}" ]' && echo "$0: invalid option $1" 1>&2 && exit 1;
      old_value="(eval echo \\$$name)";
      if [ "${old_value}" == "true" ] || [ "${old_value}" == "false" ]; then
        was_bool=true;
      else
        was_bool=false;
      fi

      # Set the variable to the right value-- the escaped quotes make it work if
      # the option had spaces, like --cmd "queue.pl -sync y"
      eval "${name}=\"$2\"";

      # Check that Boolean-valued arguments are really Boolean.
      if $was_bool && [[ "$2" != "true" && "$2" != "false" ]]; then
        echo "$0: expected \"true\" or \"false\": $1 $2" 1>&2
        exit 1;
      fi
      shift 2;
      ;;

    *) break;
  esac
done


rm -rf nohup.out
rm -rf logs/
mkdir -p logs/
mkdir -p /data/tianxing/update_stream_wav


if [ "${environment}" == "sz" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://10.75.27.200:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "dev" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://10.20.251.7:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "gz" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://10.20.251.7:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "hk" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://10.52.66.97:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "mx" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://127.0.0.1:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "sg" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://127.0.0.1:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "sea-id" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://172.16.0.225:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "id" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://10.62.254.9:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

elif [ "${environment}" == "vi" ]; then
  nohup \
  ./${build_dir}/CallMonitor \
  --http_port ${http_port} \
  --asr_event_http_host_port "http://127.0.0.1:8002" \
  --call_monitor_stderrthreshold=0 \
  --call_monitor_log_dir=./logs/ \
  > nohup.out &

fi
