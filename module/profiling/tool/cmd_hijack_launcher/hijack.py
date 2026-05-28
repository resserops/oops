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
    critical_files = (wrapper_name, setup_script_name, "config.yaml")

    # 参数解析
    program = argparse.ArgumentParser(description="deploy symlinks for command hijacking to the specified prefix directory")
    program.add_argument("-p", "--prefix", default=os.path.expanduser(os.path.join("~", ".local", "bin", os.path.splitext(wrapper_name)[0])), help=f"installation prefix directory")
    program.add_argument("-c", "--clean", action="store_true", help=f"clean up obsolete hijack symlinks in the prefix directory that are not specified in the current command list")
    program.add_argument("-f", "--force", action="store_true", help=f"force deployment by overwriting existing files, symlinks, or directories if conflicts occur")
    program.add_argument("commands", nargs="*", help="commands to hijack. must be executable in the system PATH")
    args = program.parse_args()

    # 打印基础信息
    script_dir = os.path.dirname(os.path.realpath(__file__))
    prefix_dir = os.path.abspath(args.prefix)

    print(f"Script dir: {script_dir}")
    print(f"Prefix dir: {prefix_dir}\n")

    # 创建prefix（安装路径）
    if os.path.exists(prefix_dir) or os.path.islink(prefix_dir):
        if not os.path.isdir(prefix_dir):
            if args.force:
                try:
                    os.remove(prefix_dir)
                    os.makedirs(prefix_dir, exist_ok=True)
                except OSError as e:
                    print(f"Error: failed to force replace prefix directory '{prefix_dir}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
                    sys.exit(1)
            else:
                print(f"Error: prefix path exists but is not a directory '{prefix_dir}'. use -f to overwrite", file=sys.stderr)
                sys.exit(1)
    else:
        try:
            os.makedirs(prefix_dir, exist_ok=True)
        except OSError as e:
            print(f"Error: cannot create directory '{prefix_dir}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
            sys.exit(1)

    # 复制关键脚本到prefix
    def copy_to_prefix(f):
        src = os.path.join(script_dir, f)
        # 检查src存在
        if not os.path.exists(src):
            print(f"Error: critical file '{f}' not found, hijack failed", file=sys.stderr)
            sys.exit(1)

        dst = os.path.join(prefix_dir, f)
        if os.path.exists(dst) or os.path.islink(dst):
            if os.path.isfile(dst) and filecmp.cmp(src, dst):
                # 如果已存在文件相同，直接跳过
                return
            if args.force:
                try:
                    remove_entry(dst)
                except OSError as e:
                    print(f"Error: failed to force replace critical file '{f}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)
                    return
            else:
                print(f"Warning: critical file '{f}' already exists. use -f to overwrite", file=sys.stderr)
                return

        try:
            shutil.copy2(src, dst) # 使用copy2保留原始文件权限
        except OSError as e:
            print(f"Error: failed to copy critical file. (code: {e.errno}, {e.lower()})", file=sys.stderr)
            return

    for f in critical_files:
        copy_to_prefix(f)

    # 清理未在commands中的软链接
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

    # 设置软链接
    succ_cmds = []
    for cmd in args.commands:
        if cmd == wrapper_name:
            print(f"Warning: skipping self-hijack of '{cmd}'", file=sys.stderr)
            continue

        if cmd == "python" or cmd == "python3":
            print(f"Warning: skipping hijack command '{cmd}' to prevent infinite recursion loop", file=sys.stderr)
            continue

        if not shutil.which(cmd):
            print(f"Warning: command '{cmd}' not found in system PATH. skipping hijack", file=sys.stderr)
            continue

        try:
            # 不允许劫持shell内建命令和关键字
            res = subprocess.run(["bash", "-c", f"type -t {cmd}"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
            cmd_type = res.stdout.strip()

            if cmd_type == "keyword":
                print(f"Warning: skipping hijack command '{cmd}' because it is a shell keyword", file=sys.stderr)
                continue
            if cmd_type == "builtin":
                print(f"Warning: skipping hijack command '{cmd}' because it is a shell builtin command", file=sys.stderr)
                continue
        except OSError as e:
            print(f"Error: failed to check command type for '{cmd}'. (code: {e.errno}, {e.lower()})", file=sys.stderr)

        link_name = os.path.join(prefix_dir, cmd)
        if os.path.islink(link_name) and os.readlink(link_name) == link_target:
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
