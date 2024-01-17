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
function stop() {
  pids=$(ps -e -o pid,cmd | grep -w "${cmdline}" | grep -v "grep" | awk '{print $1}')

  array=("${pids}")
  for pid in "${array[@]}"; do
    log_info "stop ${server_name}: pid=${pid}"

    kill -15 "${pid}"
  done
}

function del_cron() {
    item="${script_dir}/check.sh >> ${logs_dir}/check.log 2>&1"
    exist=$(crontab -l | grep "$item" | grep -v "#" | wc -l)
    if [ "$exist" != "0" ]; then
        log_info "del cron for ${server_name}"

        cron=$(mktemp)
        crontab -l | grep -v "$item" > "${cron}"
        crontab "${cron}"
        rm -f "${cron}"
    fi
}


# run
del_cron
stop
