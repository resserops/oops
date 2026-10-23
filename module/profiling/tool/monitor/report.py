#!/usr/bin/env python3
import argparse
import math
import sys

from dataclasses import dataclass, field
from datetime import datetime, timedelta
from enum import Enum
from glob import glob as glob_match
from pathlib import Path

# 全局变量
AUTO_RESOLUTION = 960


# 指标定义
class Merge(str, Enum):
    MEAN = "mean"  # 周期均值/瞬时采样
    MIN = "min"    # 周期内极小值
    MAX = "max"    # 周期内极大值


@dataclass(frozen=True)
class Metric:
    name: str                  # 数据列名称
    merge: Merge = Merge.MEAN  # 数据点合并方式
    step: bool = False         # 是否使用阶梯图（默认折线图）


CPU_EQ_CORES = Metric("EqCPUs", step=True)
CPU_USAGE = Metric("%Usage", step=True)
CPU_AVG_GHZ = Metric("AvgGHz", step=True)
CPU_MIN_GHZ = Metric("MinGHz", merge=Merge.MIN, step=True)
RSS = Metric("RSS(G)")
HWM = Metric("HWM(G)", merge=Merge.MAX)
SWAP = Metric("Swap(G)")
METRICS = (CPU_EQ_CORES, CPU_USAGE, CPU_AVG_GHZ, CPU_MIN_GHZ, RSS, HWM, SWAP)


# 存储单个测量结果文件
@dataclass
class FileData:
    raw_file_name: str
    pid: int = 0
    interval: float = 0.0  # 必须大于0，parse时校验
    start_time: datetime = None
    metric_data: dict = field(default_factory=dict)

    # 计算得到
    downsample_rate: int = 0  # 将原始数据的N个点合并为1个点

    def metric_data_size(self):
        if not self.metric_data:
            return 0
        return len(next(iter(self.metric_data.values())))

    def end_time(self):
        return self.start_time + timedelta(seconds=self.metric_data_size() * self.interval)

    def duration(self):
        return self.end_time() - self.start_time


# 输入文件搜索
def search_valid_input(inputs, dirs):
    literal_input = []
    patterns = []
    for input_ in inputs:
        input_path = Path(input_)
        if input_path.is_absolute():
            if "*" in input_ or "?" in input_:
                # glob绝对路径
                patterns.append(input_)
            else:
                # 普通绝对路径
                literal_input.append(input_path)
        else:
            for dir_ in dirs:
                if "*" in input_ or "?" in input_:
                    # glob相对路径，那么在dir中递归查找glob
                    patterns.append(str(Path(dir_) / "**" / input_path))
                    patterns.append(str(Path(dir_) / input_path))
                else:
                    # 普通相对路径，仅拼接
                    literal_input.append(Path(dir_) / input_path)

    # glob匹配，recursive=True使**生效
    for pattern in patterns:
        for path in glob_match(pattern, recursive=True):
            literal_input.append(Path(path))

    # 经典化路径并去重
    valid_input = []
    for path in literal_input:
        if path.is_file():
            valid_input.append(path.resolve())
        else:
            print(f"warning: bad path. ignored: {path}", file=sys.stderr)

    return sorted(set(valid_input))


# 原始数据解析
def parse_value_after_prefix(line, prefix):
    pos = line.find(prefix)
    if pos == -1:
        return False, None
    remain = line[pos + len(prefix):].lstrip()

    # 解析前导浮点数
    end = 0
    while end < len(remain) and (remain[end] in "+-.0123456789eE"):
        end += 1
    # 排除仅包含符号/小数点的极端情况，交给float抛异常兜底
    try:
        return True, float(remain[:end])
    except ValueError:
        return False, None


def parse_raw_data(path, entry):
    # 解析阶段控制变量
    step = "META"
    metric_num = 0
    columns = []

    for buf in path.read_text().splitlines():
        line = buf.strip()
        if not line:
            continue

        if step == "META":
            matched, value = parse_value_after_prefix(line, "tracked pid:")
            if matched:
                entry.pid = int(value)
                continue
            matched, value = parse_value_after_prefix(line, "interval:")
            if matched:
                entry.interval = value
                continue
            pos = line.find("timestamp:")
            if pos != -1:
                timestamp_str = line[pos + len("timestamp:"):].strip()
                entry.start_time = datetime.strptime(timestamp_str[:26], "%Y-%m-%d %H:%M:%S.%f")
                continue
            if "monitoring results:" in line:
                step = "HEADER_ROW"
        elif step == "HEADER_ROW":
            names = [token.strip() for token in line.split(", ")]
            if not names or names[0] != "#":
                raise ValueError(f"missing '#' row index column in {path}")
            metric_num = len(names) - 1
            for name in names[1:]:
                metric_entry = next((m for m in METRICS if m.name == name), None)
                if metric_entry is None:
                    raise ValueError(f"unknown metric column '{name}' in {path}")
                if metric_entry in entry.metric_data:
                    raise ValueError(f"duplicate metric column '{name}' in {path}")
                entry.metric_data[metric_entry] = []
                columns.append(metric_entry)
            step = "DATA_ROW"
        else:  # DATA_ROW
            tokens = [token.strip() for token in line.split(", ")]
            if metric_num == 0 or metric_num + 1 != len(tokens):
                raise ValueError(f"bad metric num in {path}")
            for i, token in enumerate(tokens[1:]):  # 第0列为行序号
                entry.metric_data[columns[i]].append(float(token))

    if step != "DATA_ROW":
        raise ValueError(f"bad raw data format in {path}")


# 时间轴对齐
def align_timeline(raw_data_table, range_start, range_end, resolution):
    # 计算所有raw data的公共start time
    total_start_time = min(entry.start_time for entry in raw_data_table)

    # 计算timeline range截断
    for entry in raw_data_table:
        if entry.interval <= 0:
            raise ValueError(f"bad interval in {entry.raw_file_name}")
        size = entry.metric_data_size()
        if size == 0:
            continue

        # 计算从total_start_time到当前raw data左右边界的duration
        raw_l = (entry.start_time - total_start_time).total_seconds()
        raw_r = (entry.end_time() - total_start_time).total_seconds()

        # 与range取交集，total_start_time到range左右边界的duration
        range_l = max(raw_l, range_start)
        range_r = min(raw_r, range_end)
        if range_r < range_l:
            raise ValueError(f"bad range slice in {entry.raw_file_name}")

        # 计算range在metric data中的左右下标，左闭右开
        # 与C++版一致：先对秒数向上取整，再除以interval后截断
        range_l_idx = max(int(math.ceil(range_l - raw_l) / entry.interval), 0)
        range_r_idx = min(int(math.ceil(range_r - raw_l) / entry.interval), size)
        if range_l_idx > range_r_idx:
            raise ValueError(f"bad range slice in {entry.raw_file_name}")

        # range切片
        entry.start_time += timedelta(seconds=range_l_idx * entry.interval)
        entry.metric_data = {
            metric_entry: values[range_l_idx:range_r_idx]
            for metric_entry, values in entry.metric_data.items()
        }

    # 重新计算range切片后的公共start time和end time，获取数据区间总长，用于计算向下采样倍率
    total_range_start_time = min(entry.start_time for entry in raw_data_table)
    total_range_end_time = max(entry.end_time() for entry in raw_data_table)
    total_range_duration = (total_range_end_time - total_range_start_time).total_seconds()

    # 根据时间轴分辨率，计算每份raw data的向下采样倍率
    for entry in raw_data_table:
        # 每downsample_rate个点合并为1个点，如何合并由每项指标自己定义
        range_resolution = int(resolution * entry.duration().total_seconds() / total_range_duration)
        if range_resolution < 1:
            range_resolution = 1
        entry.downsample_rate = entry.metric_data_size() // range_resolution


def derive_ratio(raw_data_table, numer, denom):
    for entry in raw_data_table:
        numer_values = entry.metric_data.get(numer)
        denom_values = entry.metric_data.get(denom)
        if not numer_values or not denom_values:
            continue
        ratio = next((n / d for d, n in zip(denom_values, numer_values) if d > 0), None)
        if ratio is not None:
            return ratio
    return None


def merge_series(values, rate, kind):
    if rate <= 1:
        return values
    merged = []
    i = 0
    while i + rate - 1 < len(values):
        window = values[i: i + rate]
        if kind == Merge.MIN:
            merged.append(min(window))
        elif kind == Merge.MAX:
            merged.append(max(window))
        else:
            merged.append(sum(window) / rate)
        i += rate
    return merged


def make_figure(num_axes):
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(
        num_axes,
        1,
        sharex=True,
        figsize=(16, max(min(9, 3 * num_axes), num_axes)),
    )
    if num_axes == 1:
        axes = [axes]
    return fig, axes


def style_axis(ax, ylim):
    ax.margins(x=0)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.set_ylim(-ylim / 20, ylim + ylim / 20)  # 对齐每个子图y坐标范围


def label_axis(ax, i, r_entry, unit):
    ax.set_ylabel(unit)
    ax.set_title(f"#{i}  Pid: {r_entry.pid} File: {r_entry.raw_file_name}")
    ax.legend(loc="upper right")


def finish_figure(fig, axes, title, file_name, show):
    import matplotlib.pyplot as plt

    fig.suptitle(title, fontsize=16, fontweight="bold")
    axes[-1].set_xlabel("Time (s)")
    fig.tight_layout()
    fig.savefig(file_name, dpi=100, bbox_inches="tight")

    if show:
        plt.show()
    plt.close("all")


def plot_group(raw_data_table, total_start_time, show, title, file_name, metric_entries, unit, right_unit=None):
    # 收集各raw data中本组可用指标
    entries = []
    for r_entry in raw_data_table:
        metrics = [(metric_entry, r_entry.metric_data[metric_entry])
                   for metric_entry in metric_entries if r_entry.metric_data.get(metric_entry)]
        if metrics:
            entries.append((r_entry, metrics))
    if not entries:
        return

    # 最大数值
    max_value = max(
        (value for _, metrics in entries for _, values in metrics for value in values), default=-math.inf
    )
    ylim = max(1.0, max_value)

    fig, axes = make_figure(len(entries))
    for i, (r_entry, metrics) in enumerate(entries):
        offset = (r_entry.start_time - total_start_time).total_seconds()

        # 构造时间数组，由于offset差异，每个raw data需要单独构造
        times = [offset + j * r_entry.interval for j in range(len(metrics[0][1]))]

        ax = axes[i]
        for metric_entry, values in metrics:
            if metric_entry.step:
                ax.step(times, values, where="post", label=metric_entry.name, linewidth=1.5)
            else:
                ax.plot(times, values, label=metric_entry.name, linewidth=1.5)
        style_axis(ax, ylim)
        label_axis(ax, i, r_entry, unit)

        # 右轴与左轴共用一条曲线，按固定倍率缩放
        if right_unit is not None:
            right_name, ratio = right_unit
            if ratio is not None:
                ax_r = ax.twinx()
                lo, hi = ax.get_ylim()
                ax_r.set_ylim(lo * ratio, hi * ratio)
                ax_r.set_ylabel(right_name)

    finish_figure(fig, axes, title, file_name, show)


def parse_args():
    parser = argparse.ArgumentParser(prog="report", description="process and visualize monitor raw data")
    parser.add_argument("input", help="comma-separated sequence of raw data file")
    parser.add_argument("-d", "--dir", default=".", help="the root directory to search for files matching the 'input' pattern")
    parser.add_argument(
        "--resolution",
        default="auto",
        help="number of data points plotted on timeline ('auto', 'max' or an integer)",
    )
    parser.add_argument(
        "-r",
        "--range",
        default=":",
        help="subset range for the plot timeline specified as 'start:end', where boundaries must be real or empty",
    )
    parser.add_argument("--show", action="store_true", help="display the generated plot in a GUI window")
    args = parser.parse_args()

    # input
    args.input = [token for token in args.input.split(",")]

    # --dir
    args.dir = [token for token in args.dir.split(",")]
    for dir_ in args.dir:
        if not Path(dir_).is_dir():
            print(f"error: '{dir_}' in --dir is not a directory", file=sys.stderr)
            sys.exit(1)
    if not args.dir:
        print("error: no valid directories provided in --dir", file=sys.stderr)
        sys.exit(1)

    # --resolution
    if args.resolution == "auto":
        args.resolution = AUTO_RESOLUTION
    elif args.resolution == "max":
        args.resolution = sys.maxsize
    else:
        try:
            args.resolution = int(args.resolution)
        except ValueError:
            print(f"error: unexpected resolution '{args.resolution}'", file=sys.stderr)
            sys.exit(1)

    # --range
    range_tokens = [token.strip() for token in args.range.split(":")]
    if len(range_tokens) != 2:
        print(f"error: unexpected range format '{args.range}'", file=sys.stderr)
        sys.exit(1)

    range_start = 0.0
    range_end = math.inf
    if range_tokens[0]:
        try:
            range_start = float(range_tokens[0])
        except ValueError:
            print(f"error: unexpected range start '{range_tokens[0]}'", file=sys.stderr)
            sys.exit(1)
        if range_start < 0:
            print(f"error: range start '{range_start}' must be >= 0", file=sys.stderr)
            sys.exit(1)
    if range_tokens[1]:
        try:
            range_end = float(range_tokens[1])
        except ValueError:
            print(f"error: unexpected range end '{range_tokens[1]}'", file=sys.stderr)
            sys.exit(1)
        if range_end < range_start:
            print(f"error: range end '{range_end}' must be >= range start '{range_start}'", file=sys.stderr)
            sys.exit(1)
    args.range_start = range_start
    args.range_end = range_end

    return args


def main():
    # 解析命令行参数
    args = parse_args()

    # 搜索日志文件
    valid_input = search_valid_input(args.input, args.dir)

    # 解析数据
    raw_data_table = []
    for path in valid_input:
        entry = FileData(raw_file_name=path.name)
        parse_raw_data(path, entry)
        raw_data_table.append(entry)

    if not raw_data_table:
        return

    # 对齐时间轴，range裁剪，分辨率缩放
    # 注意：total_start_time需在切片前计算，对应C++版AlignTimeline开头的TOTAL_START_TIME
    total_start_time = min(entry.start_time for entry in raw_data_table)
    align_timeline(raw_data_table, args.range_start, args.range_end, args.resolution)

    # 根据downsample_rate合并数据点
    for entry in raw_data_table:
        if entry.downsample_rate > 1:
            rate = entry.downsample_rate
            entry.metric_data = {
                metric_entry: merge_series(values, rate, metric_entry.merge)
                for metric_entry, values in entry.metric_data.items()
            }
            entry.interval *= rate

    # 绘制CPU利用率、CPU主频、内存时序图
    plot_group(
        raw_data_table,
        total_start_time,
        args.show,
        "CPU Usage Time Series",
        "cpu_usage.png",
        (CPU_EQ_CORES,),
        unit="EqCores",
        right_unit=("%Usage", derive_ratio(raw_data_table, CPU_USAGE, CPU_EQ_CORES)),
    )
    plot_group(
        raw_data_table,
        total_start_time,
        args.show,
        "CPU Frequency Time Series",
        "cpu_freq.png",
        (CPU_AVG_GHZ, CPU_MIN_GHZ),
        unit="GHz",
    )
    plot_group(
        raw_data_table,
        total_start_time,
        args.show,
        "Memory Time Series",
        "memory.png",
        (RSS, HWM, SWAP),
        unit="G",
    )


if __name__ == "__main__":
    main()
