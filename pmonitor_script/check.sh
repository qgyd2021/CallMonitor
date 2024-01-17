#!/bin/bash

cmdline=
server_name=

logs_dir="./logs"

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


if [ -z "${server_name}" ]; then
  echo "'--server_name' is required." && exit 1;
fi

if [ -z "${cmdline}" ]; then
  echo "'--cmdline' is required." && exit 1;
fi

# ${str:a:b} means extracting b characters starting from string a
# ${0:a:b} means extracting b characters starting from string a from ${0}
# ${0} is "start.sh" in "sh start.sh"

# get the absolute dir where the start.sh file is located.
if [ "${0:0:1}" == "/" ]; then
  # command like "sh /p_monitor_script/start.sh"
  script_dir=$(dirname "$0")
else
  # command like "sh p_monitor_script/start.sh"
  script_dir=$(pwd)/$(dirname "$0")
fi


# functions
function log_date() {
  date "+%Y-%m-%dT%H:%M:%S"
}

function log_error() {
  echo -e "\033[31m\033[01m$(log_date)\terror\t$1 \033[0m"
}

function log_info() {
  echo -e "\033[32m\033[01m$(log_date)\tinfo\t$1 \033[0m"
}

function start() {
    log_info "start ${server_name}"

    if [ ! -d ${logs_dir} ]; then
        mkdir ${logs_dir}
    fi

    ulimit -n 102400
    nohup "${cmdline}" >> ${logs_dir}/stdout 2>&1 &
}


pids=$(ps -e -o pid,cmd | grep -w "${cmdline}" | grep -v "grep" | awk '{print $1}')


array=("${pids}")


if [ "${#array[@]}" == "0" ]; then
  "${script_dir}/alarm.sh" "${server_name}" "dev_2 异常退出"
  start
  pids=$(ps -e -o pid,cmd | grep -w "${cmdline}" | grep -v "grep" | awk '{print $1}')
  "${script_dir}/alarm.sh" "${server_name}" "重启成功,进程id为${pids}"
fi
