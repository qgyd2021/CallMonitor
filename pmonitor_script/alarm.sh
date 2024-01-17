#!/bin/bash


server_name=
message=

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

if [ -z "${message}" ]; then
  echo "'--message' is required." && exit 1;
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


d="$(date '+%Y-%m-%d %H:%M:%S')"
ip="$(curl ifconfig.me)"


content=$(cat <<- EOF
# 服务异常告警
## 服务[$server_name]()
- **发生时间:** [$d]()
- **IP:** $ip
- $message
- 麻烦确认
EOF
)

echo "${content}"

"${script_dir}/send_wecom" \
-webhook=https://qyapi.weixin.qq.com/cgi-bin/webhook/send \
-secretKey=fd86eb80-6fe0-4fb0-9f3b-3277268ea82d \
-msgType=markdown \
-markdownContent="${content}"
