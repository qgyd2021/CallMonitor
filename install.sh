#!/usr/bin/env bash

# sh install.sh --stage -1 --stop_stage -1 --system_version windows
# sh install.sh --stage -1 --stop_stage -1 --system_version centos
# sh install.sh --stage 1 --stop_stage 1 --system_version centos

gcc_version=11.1.0
python_version=3.8.10
system_version=centos

verbose=true;
stage=-1
stop_stage=3

work_dir="$(pwd)"

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


yum install -y bzip2 git lrzsz wget vim


if [ ${stage} -le -1 ] && [ ${stop_stage} -ge -1 ]; then
  $verbose && echo "stage -1: download models"
  cd "${work_dir}" || exit 1;

  mkdir -p trained_models && cd trained_models || exit 1;

  model_name_array=(
    cnn_voicemail_id_20230519
    cnn_voicemail_jp_20221117
    cnn_voicemail_mx_20230519
    cnn_voicemail_th_20221107
    cnn_voicemail_tw_20230519
    cnn_voicemail_us_20221109
  )

  for model_name in ${model_name_array[*]}
  do
    if [ ! -d "${model_name}" ]; then
      wget -c "https://huggingface.co/qgyd2021/cnn_voicemail/resolve/main/${model_name}.zip"
      unzip "${model_name}.zip"
      rm "${model_name}.zip"
    fi
  done
fi


if [ ${stage} -le 0 ] && [ ${stop_stage} -ge 0 ]; then
  $verbose && echo "stage 0: install python"
  cd "${work_dir}" || exit 1;

  sh ./script/install_python.sh --python_version "${python_version}" --system_version "${system_version}"
fi


if [ ${stage} -le 1 ] && [ ${stop_stage} -ge 1 ]; then
  $verbose && echo "stage 1: create virtualenv"
  /usr/local/python-${python_version}/bin/pip3 install virtualenv
  mkdir -p /data/local/bin
  cd /data/local/bin || exit 1;
  # source /data/local/bin/CallMonitor/bin/activate
  /usr/local/python-${python_version}/bin/virtualenv CallMonitor

fi


if [ ${stage} -le 2 ] && [ ${stop_stage} -ge 2 ]; then
  $verbose && echo "stage 2: install cmake"
  cd "${work_dir}" || exit 1;

  sh ./script/install_cmake.sh --system_version "${system_version}"
fi


if [ ${stage} -le 3 ] && [ ${stop_stage} -ge 3 ]; then
  $verbose && echo "stage 3: install gcc"
  cd "${work_dir}" || exit 1;

  sh ./script/install_gcc.sh --gcc_version "${gcc_version}" --system_version "${system_version}"
fi


if [ ${stage} -le 3 ] && [ ${stop_stage} -ge 3 ]; then
  yum install -y gdb

  debuginfo-install -y glibc-2.17-326.el7_9.x86_64

  # vim /root/.gdbinit
  # add-auto-load-safe-path /usr/lib64/libstdc++.so.6.0.29-gdb.py
  # set auto-load safe-path /
  echo -e "add-auto-load-safe-path /usr/lib64/libstdc++.so.6.0.29-gdb.py\nset auto-load safe-path /" > /root/.gdbinit

fi
