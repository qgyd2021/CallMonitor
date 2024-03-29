#!/usr/bin/env bash

# MSVC cmake.
# sh build.sh --stage 0 --stop_stage 0 --system_version windows
# sh build.sh --stage 0 --stop_stage 0 --system_version centos

system_version=windows;
verbose=true;
stage=0
stop_stage=0

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


if [ ${stage} -le 0 ] && [ ${stop_stage} -ge 0 ]; then
  $verbose && echo "stage 0: build"
  cd "${work_dir}" || exit 1;

  if [ "$(grep -c ^processor /proc/cpuinfo)" -lt 8 ]; then
    # cmake -B build (Use half the CPUs)
    echo "cpu count less than 8, would not to compile it.";
  else
    # cmake -B build (Use half the CPUs)
    cmake --build ./build --target CallMonitor -j "$(($(grep -c ^processor /proc/cpuinfo) / 2))"
  fi

  if [ "${system_version}" == "windows" ]; then
    cp ./build/Debug/CallMonitor.exe ./build/CallMonitor.exe
  fi

  if [ "${system_version}" == "windows" ]; then
    echo "run './build/CallMonitor.exe' to test. "
  elif [ "${system_version}" == "centos" ]; then
    echo "run './build/CallMonitor' to test. "
  fi

fi



