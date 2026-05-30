#!/usr/bin/env python3
import argparse
import os
import sys
import shutil
import filecmp
import subprocess

def remove_entry(entry):
    if os.path.isdir(entry) and not os.path.islink(entry):
        shutil.rmtree(entry)
    else:
        os.remove(entry)

def main():
    wrapper_name = "cmd_hijack_launcher.py"
    setup_script_name = "setup.sh"

    # 参数解析
    program = argparse.ArgumentParser(description="deploy symlinks for command hijacking to the specified prefix directory")
    program.add_argument("-c", "--clean", action="store_true", help=f"clean up obsolete hijack symlinks in the prefix directory that are not specified in the current command list")
    program.add_argument("targets", nargs="*", help="targets to hijack. must be commands in the system PATH or executable file paths")
    args = program.parse_args()

    # 打印基础信息
    script_dir = os.path.dirname(os.path.realpath(__file__))
    print(f"Script dir: {script_dir}")
    
    cmds = []
    exes = []
    for target in args.targets:
        if os.path.dirname(target):
            exes.append(target)
        else:
            if target == wrapper_name:
                print(f"Warning: skipping self-hijack of command '{target}'", file=sys.stderr)
                continue
            if target == "python" or target == "python3":
                print(f"Warning: skipping hijack command '{target}' to prevent infinite recursion loop", file=sys.stderr)
                continue
            if not shutil.which(target):
                print(f"Warning: command '{target}' not found in system PATH. skipping hijack", file=sys.stderr)
                continue

            # 不允许劫持shell内建命令和关键字
            res = subprocess.run(["bash", "-c", f"type -t {target}"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
            cmd_type = res.stdout.strip()

            if cmd_type == "keyword":
                print(f"Warning: skipping hijack command '{target}' because it is a shell keyword", file=sys.stderr)
                continue
            if cmd_type == "builtin":
                print(f"Warning: skipping hijack command '{target}' because it is a shell builtin command", file=sys.stderr)
                continue

            cmds.append(target)

    # 清理命令
    exists_cmds = []
    link_target = os.path.join(".", wrapper_name)

    try:
        for entry in os.scandir(prefix_dir):
            if entry.is_symlink() and entry.name not in args.commands and os.readlink(entry.path) == link_target:
                if args.clean:
                    try:
                        os.remove(entry.path)
                        print(f"Command '{entry.name}' unhijacked")
                    except OSError:
                        print(f"Error: failed to remove symlink for '{entry.name}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
                        continue
                else:
                    exists_cmds.append(entry.name)
    except OSError as e:
        print(f"Error: failed to scan prefix directory for cleanup. (code: {e.errno}, {e.lower()})", file=sys.stderr)


    # 劫持命令
    if len(cmds) > 0:
        # 创建cmd_dir
        cmd_dir = os.path.join(script_dir, "cmds")
        if os.path.islink(cmd_dir):
            os.unlink(cmd_dir)
        elif os.path.isfile(cmd_dir):
            os.remove(cmd_dir)

        os.makedirs(cmd_dir, exist_ok=True)

        # 生成劫持软连接
        link_target = os.path.join("..", wrapper_name)
        for cmd in cmds:    
            link_name = os.path.join(cmd_dir, cmd)
            if os.path.islink(link_name) and os.readlink(link_name) == link_target:
    
    # 劫持具体文件



        
        
            # 已经存在且正确的链接可以忽略
            exists_cmds.append(cmd)
            continue

        if os.path.exists(link_name) or os.path.islink(link_name):
            # 不是链接，或者是链接但并未链接到目标
            if args.force:
                try:
                    remove_entry(link_name)
                except OSError as e:
                    print(f"Error: failed to force replace hijack link '{cmd}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
                    continue
            elif os.path.islink(link_name):
                print(f"Error: hijack link '{f}' already exists. use -f to overwrite '{cmd}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
                continue

        # 建立软链接
        try:
            os.symlink(link_target, link_name)
            succ_cmds.append(cmd)
        except OSError as e:
            print(f"Error: failed to create symlink for '{cmd}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)

    exists_cmds.sort()
    for cmd in exists_cmds:
        print(f"Command '{cmd}' already hijacked")
    
    succ_cmds.sort()
    for cmd in succ_cmds:
        print(f"Command '{cmd}' successfully hijacked")

    if len(succ_cmds) == 0 and len(exists_cmds) == 0:
        print("No commands hijacked")
    else:
        print(f"To enable and activate the hijacked commands, please run: 'source {os.path.join(prefix_dir, setup_script_name)}'")

if __name__ == "__main__":
    main()
