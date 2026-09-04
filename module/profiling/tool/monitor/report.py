#!/usr/bin/env python3
import argparse
import math
import sys
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from glob import glob as glob_match
from pathlib import Path

AUTO_RESOLUTION = 960
METRIC_TABLE = {
    "cpu": "%Cpu",
    "rss": "Rss(G)",
    "hwm": "Hwm(G)",
    "swap": "Swap(G)",
}


@dataclass
class MetricDataEntry:
    name: str
    values: list = field(default_factory=list)


@dataclass
class RawDataEntry:
    raw_file_name: str
    pid: int = 0
    interval: float = 0.0  # 必须大于0，parse时校验
    start_time: datetime = None
    metric_data_table: list = field(default_factory=list)

    # 计算得到
    downsample_rate: int = 0  # 将原始数据的N个点合并为1个点

    def metric_data_size(self):
        if not self.metric_data_table:
            return 0
        return len(self.metric_data_table[0].values)

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
            print(f"Warning: bad path. ignored: {path}", file=sys.stderr)

    return sorted(set(valid_input))


# 原始数据解析
def parse_value_after_prefix(line, prefix):
    """提取prefix之后的数值，返回(匹配且解析成功, 值)；仅匹配未解析成功返回(False, None)"""
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


def str_to_timestamp(sv):
    sv = sv.strip()
    if len(sv) < 26:
        raise ValueError("size too small for timestamp format")
    # 解析"YYYY-mm-dd HH:MM:SS"和微秒
    return datetime.strptime(sv[:26], "%Y-%m-%d %H:%M:%S.%f")


def parse_raw_data(path, entry):
    # 解析阶段控制变量
    step = "META"
    metric_num = 0

    for buf in path.read_text().splitlines():
        line = buf.strip()
        if not line:
            continue

        if step == "META":
            matched, value = parse_value_after_prefix(line, "Tracked pid:")
            if matched:
                entry.pid = int(value)
                continue
            matched, value = parse_value_after_prefix(line, "Interval:")
            if matched:
                entry.interval = value
                continue
            pos = line.find("Timestamp:")
            if pos != -1:
                entry.start_time = str_to_timestamp(line[pos + len("Timestamp:"):])
                continue
            if "Monitoring results:" in line:
                step = "HEADER_ROW"
        elif step == "HEADER_ROW":
            for token in line.split(", "):
                entry.metric_data_table.append(MetricDataEntry(name=token.strip()))
            metric_num = len(entry.metric_data_table)
            step = "DATA_ROW"
        else:  # DATA_ROW
            tokens = [token.strip() for token in line.split(", ")]
            if metric_num == 0 or metric_num != len(tokens):
                raise ValueError(f"bad metric num in {path}")
            for i, token in enumerate(tokens):
                entry.metric_data_table[i].values.append(float(token))

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
        for metric in entry.metric_data_table:
            metric.values = metric.values[range_l_idx:range_r_idx]

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


# 绘图
def elide(s, n):
    if len(s) <= n:
        return s
    if n <= 3:
        return "." * n
    return s[: n - 3] + "..."


def plot_cpu(raw_data_table, total_start_time, show):
    import matplotlib.pyplot as plt

    entries = [
        (entry, metric)
        for entry in raw_data_table
        for metric in entry.metric_data_table
        if metric.name == METRIC_TABLE["cpu"] and metric.values
    ]
    if not entries:
        return

    # 根据downsample_rate，合并数据点，cpu利用率采取平均值
    for r_entry, m_entry in entries:
        if r_entry.downsample_rate <= 1:
            # 不需要合并
            continue

        rate = r_entry.downsample_rate
        merged = []
        i = 0
        while i + rate - 1 < len(m_entry.values):
            merged.append(sum(m_entry.values[i: i + rate]) / rate)
            i += rate
        m_entry.values = merged
        r_entry.interval *= rate

    # 最大数值
    max_value = max((value for _, m_entry in entries for value in m_entry.values), default=-math.inf)

    fig, axes = plt.subplots(
        len(entries),
        1,
        sharex=True,
        figsize=(16, max(min(9, 3 * len(entries)), len(entries))),
    )
    if len(entries) == 1:
        axes = [axes]

    for i, (r_entry, m_entry) in enumerate(entries):
        offset = (r_entry.start_time - total_start_time).total_seconds()

        # 构造时间数组，由于offset差异，每个raw data需要单独构造
        times = [offset + j * r_entry.interval for j in range(len(m_entry.values))]

        ax = axes[i]
        # 使用step绘制阶梯图，post逻辑：第0个点，时刻是0，对应[0, itv]区间的平均CPU利用率
        ax.step(times, m_entry.values, where="post", label=m_entry.name, linewidth=1.5)
        ax.margins(x=0)
        ax.grid(True, linestyle="--", alpha=0.5)
        ylim = max(100.0, max_value)
        ax.set_ylim(-ylim / 20, ylim + ylim / 20)  # 对齐每个子图y坐标范围
        # 绘制每个子图左侧描述raw data的标签
        ax.set_ylabel(
            f"#{i}\nPid: {r_entry.pid}\n{elide(r_entry.raw_file_name, 10)}",
            rotation=0,
            labelpad=30,
            va="center",
        )
        ax.legend(loc="upper right")

    fig.suptitle("CPU Utilization Time Series (%)", fontsize=16, fontweight="bold")
    axes[-1].set_xlabel("Time (s)")
    fig.tight_layout()
    fig.savefig("cpu_usage.png", dpi=100, bbox_inches="tight")

    if show:
        plt.show()
    plt.close("all")


def report(args):
    # 搜索日志文件
    valid_input = search_valid_input(args.input, args.dir)

    # 解析数据
    raw_data_table = []
    for path in valid_input:
        entry = RawDataEntry(raw_file_name=path.name)
        parse_raw_data(path, entry)
        raw_data_table.append(entry)

    if not raw_data_table:
        return

    # 对齐时间轴，range裁剪，分辨率缩放
    # 注意：total_start_time需在切片前计算，对应C++版AlignTimeline开头的TOTAL_START_TIME
    total_start_time = min(entry.start_time for entry in raw_data_table)
    align_timeline(raw_data_table, args.range_start, args.range_end, args.resolution)

    # 绘制CPU利用率折线图
    plot_cpu(raw_data_table, total_start_time, args.show)


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
            print(f"Error: '{dir_}' in --dir is not a directory", file=sys.stderr)
            sys.exit(1)
    if not args.dir:
        print("Error: no valid directories provided in --dir", file=sys.stderr)
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
            print(f"Error: unexpected resolution '{args.resolution}'", file=sys.stderr)
            sys.exit(1)

    # --range
    range_tokens = [token.strip() for token in args.range.split(":")]
    if len(range_tokens) != 2:
        print(f"Error: unexpected range format '{args.range}'", file=sys.stderr)
        sys.exit(1)

    range_start = 0.0
    range_end = math.inf
    if range_tokens[0]:
        try:
            range_start = float(range_tokens[0])
        except ValueError:
            print(f"Error: unexpected range start '{range_tokens[0]}'", file=sys.stderr)
            sys.exit(1)
        if range_start < 0:
            print(f"Error: range start '{range_start}' must be >= 0", file=sys.stderr)
            sys.exit(1)
    if range_tokens[1]:
        try:
            range_end = float(range_tokens[1])
        except ValueError:
            print(f"Error: unexpected range end '{range_tokens[1]}'", file=sys.stderr)
            sys.exit(1)
        if range_end < range_start:
            print(f"Error: range end '{range_end}' must be >= range start '{range_start}'", file=sys.stderr)
            sys.exit(1)
    args.range_start = range_start
    args.range_end = range_end

    return args


def main():
    report(parse_args())


if __name__ == "__main__":
    main()
