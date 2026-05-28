#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess
import datetime

def main():
    script_path = os.path.realpath(__file__)
    script_dir = os.path.dirname(script_path)
    config_path = os.path.join(script_dir, "config.yaml")

    cmd = os.path.basename(sys.argv[0])
    cmd_args = sys.argv[1:]

    # 清除自身环境变量
    env_path = os.environ.get("PATH", "")
    paths = []
    for path in env_path.split(os.path.pathsep):
        if os.path.realpath(path) != script_dir:
            paths.append(path)
    env_path_clean = os.path.pathsep.join(paths)

    # 查找被劫持的原始命令
    hijacked_cmd = shutil.which(cmd, path=env_path_clean)
    if not hijacked_cmd:
        print(f"Error: can't find hijacked command \"{hijacked_cmd}\" in system PATH", file=sys.stderr)
        sys.exit(127)

    # 打印基础信息
    pid = os.getpid()
    print(f"Pid: {pid}")
    print(f"Hijacked cmd: {cmd}")
    print(f"Current path: {os.getcwd()}")

    # 解析配置文件
    launched_cmds = []
    if os.path.isfile(config_path):
        try:
            import yaml
            with open(config_path, "r", encoding="utf-8") as f:
                config = yaml.load(f, Loader=yaml.SafeLoader)
                if config and isinstance(config, dict):
                    launched_cmds = config.get("launch", [])
        except Exception as e:
            print(f"Error: failed to load config.yaml. (code: {e.errno}. {e.lower()})", file=sys.stderr)
            exit()

    # 构建命令格式化map
    now = datetime.datetime.now()
    timestamp = str(int(now.timestamp())).zfill(10)
    datetime_fmt_chars = "YymdHMS"
    fmt = {"pid": str(pid), "cmd": cmd, "timestamp": timestamp}
    for ch in datetime_fmt_chars:
        fmt[ch] = now.strftime(f"%{ch}")

    # 发射命令
    for i, lcmd in enumerate(launched_cmds):
        if not isinstance(lcmd, str) or not lcmd.strip():
            continue

        try:
            lcmd = lcmd.format_map(fmt)
            lcmd_name = os.path.splitext(os.path.basename(lcmd.split()[0]))[0]
            log_file = f"launched-{now.strftime(f"%y%m%d%H%M%S")}-{pid}-{i}-{lcmd_name}-screen.log"
            with open(log_file, "w", encoding="utf-8") as f:
                # TODO(resserops): 是否支持shell解释，通过yaml配置决策
                subprocess.Popen(lcmd, stdout=f, stderr=subprocess.STDOUT, shell=True, close_fds=True)
                print(f"Cmd '{lcmd_name}' successfully launched")
        except Exception as e:
            print(f"Error: failed to launch command '{lcmd}' (code: {e.errno}. {e.lower()})", file=sys.stderr)

    # 执行被劫持的原始命令
    print()
    os.execv(hijacked_cmd, [hijacked_cmd] + cmd_args)

if __name__ == "__main__":
    main()
