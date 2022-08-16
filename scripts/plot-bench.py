#!/usr/bin/env python3

import csv
import math
import argparse
import collections
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser(description='plots benchmark charts')
parser.add_argument('-i', '--input-file', required=True, default='tmp/bench.dat')
parser.add_argument('-t', '--bench-type', required=True, type=int, default=64)
parser.add_argument('-o', '--output-pattern', required=True, default='images/bench-%s-64.png')
args = parser.parse_args()

infile = args.input_file
outpat = args.output_pattern

data = {}

with open(infile) as tsv:
    for tuple in zip(*[line for line in csv.reader(tsv, dialect="excel-tab")]):
        col = list(tuple)
        name = col.pop(0)
        data[name] = [float(x) for x in col]

x_data = data['size']

def make_plot(title):
    mpl.style.use('default')
    fig = plt.figure(figsize=(10, 6))
    ax1 = fig.add_subplot(1, 1, 1)
    plt.xkcd()
    ax1.set_xscale('log', base=2)
    ax1.yaxis.set_major_locator(mpl.ticker.MultipleLocator(10000))
    ax1.xaxis.set_major_locator(mpl.ticker.FixedLocator(x_data))
    ax1.grid(which = 'major', linestyle='--')
    ax1.set_xlabel("Buffer Size (Bytes)", labelpad=12, fontsize=12)
    ax1.set_ylabel("Bandwidth (MiB/sec)", labelpad=12, fontsize=12)
    plt.title(title, fontsize=12, pad=12)
    return plt

def plot_bench_zvec_scan():
    plt = make_plot('zvec-scan-block-%d' % args.bench_type)
    plt.plot(x_data, data['scan_abs_mib_sec'],      'tab:cyan',   ls='-',         marker='x', ms=4, label='scan-abs',      alpha=0.5)
    plt.plot(x_data, data['scan_rel_mib_sec'],      'tab:blue',   ls='-',         marker='x', ms=4, label='scan-rel',      alpha=0.5)
    plt.plot(x_data, data['scan_both_mib_sec'],     'tab:gray',   ls='-',         marker='x', ms=4, label='scan-both',     alpha=0.5)
    plt.legend(title="Benchmark", fontsize=9, title_fontsize=10.5)
    plt.tight_layout()
    plt.savefig(outpat % 'zvec-scan', dpi=240)

def plot_bench_zvec_synth():
    plt = make_plot('zvec-synthesize-block-%d' % args.bench_type)
    plt.plot(x_data, data['memcpy_mib_sec'],        'tab:pink',   ls='-',         marker='v', ms=4, label='memcpy',        alpha=0.5)
    plt.plot(x_data, data['synth_con_mib_sec'],     'tab:olive',  ls='-',         marker='v', ms=4, label='synth-const',   alpha=0.5)
    plt.plot(x_data, data['synth_seq_mib_sec'],     'tab:cyan',   ls='-',         marker='v', ms=4, label='synth-seq',     alpha=0.5)
    plt.legend(title="Benchmark", fontsize=9, title_fontsize=10.5)
    plt.tight_layout()
    plt.savefig(outpat % 'zvec-synth', dpi=240)

def plot_bench_zvec_encode():
    plt = make_plot('zvec-encode-block-%d' % args.bench_type)
    plt.plot(x_data, data['encode_rel_8_mib_sec'],  'tab:orange', ls=(0, (1, 1)), marker='o', ms=4, label='encode-rel-8',  alpha=0.5)
    plt.plot(x_data, data['encode_rel_16_mib_sec'], 'tab:green',  ls=(0, (1, 1)), marker='o', ms=4, label='encode-rel-16', alpha=0.5)
    plt.plot(x_data, data['encode_rel_24_mib_sec'], 'tab:red',    ls=(0, (1, 1)), marker='o', ms=4, label='encode-rel-24', alpha=0.5)
    if args.bench_type == 64:
        plt.plot(x_data, data['encode_rel_32_mib_sec'], 'tab:purple', ls=(0, (1, 1)), marker='o', ms=4, label='encode-rel-32', alpha=0.5)
        plt.plot(x_data, data['encode_rel_48_mib_sec'], 'tab:brown',  ls=(0, (1, 1)), marker='o', ms=4, label='encode-rel-48', alpha=0.5)
    plt.plot(x_data, data['encode_abs_8_mib_sec'],  'tab:orange', ls=(0, (2, 1)), marker='s', ms=4, label='encode-abs-8',  alpha=0.5)
    plt.plot(x_data, data['encode_abs_16_mib_sec'], 'tab:green',  ls=(0, (2, 1)), marker='s', ms=4, label='encode-abs-16', alpha=0.5)
    plt.plot(x_data, data['encode_abs_24_mib_sec'], 'tab:red',    ls=(0, (2, 1)), marker='s', ms=4, label='encode-abs-24', alpha=0.5)
    if args.bench_type == 64:
        plt.plot(x_data, data['encode_abs_32_mib_sec'], 'tab:purple', ls=(0, (2, 1)), marker='s', ms=4, label='encode-abs-32', alpha=0.5)
        plt.plot(x_data, data['encode_abs_48_mib_sec'], 'tab:brown',  ls=(0, (2, 1)), marker='s', ms=4, label='encode-abs-48', alpha=0.5)
    plt.legend(title="Benchmark", fontsize=9, title_fontsize=10.5)
    plt.tight_layout()
    plt.savefig(outpat % 'zvec-encode', dpi=240)

def plot_bench_zvec_decode():
    plt = make_plot('zvec-decode-block-%d' % args.bench_type)
    plt.plot(x_data, data['decode_rel_8_mib_sec'],  'tab:orange', ls=(0, (1, 0)), marker='o', ms=4, label='decode-rel-8',  alpha=0.5)
    plt.plot(x_data, data['decode_rel_16_mib_sec'], 'tab:green',  ls=(0, (1, 0)), marker='o', ms=4, label='decode-rel-16', alpha=0.5)
    plt.plot(x_data, data['decode_rel_24_mib_sec'], 'tab:red',    ls=(0, (1, 0)), marker='o', ms=4, label='decode-rel-24', alpha=0.5)
    if args.bench_type == 64:
        plt.plot(x_data, data['decode_rel_32_mib_sec'], 'tab:purple', ls=(0, (1, 0)), marker='o', ms=4, label='decode-rel-32', alpha=0.5)
        plt.plot(x_data, data['decode_rel_48_mib_sec'], 'tab:brown',  ls=(0, (1, 0)), marker='o', ms=4, label='decode-rel-48', alpha=0.5)
    plt.plot(x_data, data['decode_abs_8_mib_sec'],  'tab:orange', ls=(0, (2, 0)), marker='s', ms=4, label='decode-abs-8',  alpha=0.5)
    plt.plot(x_data, data['decode_abs_16_mib_sec'], 'tab:green',  ls=(0, (2, 0)), marker='s', ms=4, label='decode-abs-16', alpha=0.5)
    plt.plot(x_data, data['decode_abs_24_mib_sec'], 'tab:red',    ls=(0, (2, 0)), marker='s', ms=4, label='decode-abs-24', alpha=0.5)
    if args.bench_type == 64:
        plt.plot(x_data, data['decode_abs_32_mib_sec'], 'tab:purple', ls=(0, (2, 0)), marker='s', ms=4, label='decode-abs-32', alpha=0.5)
        plt.plot(x_data, data['decode_abs_48_mib_sec'], 'tab:brown',  ls=(0, (2, 0)), marker='s', ms=4, label='decode-abs-48', alpha=0.5)
    plt.legend(title="Benchmark", fontsize=9, title_fontsize=10.5)
    plt.tight_layout()
    plt.savefig(outpat % 'zvec-decode', dpi=240)

plot_bench_zvec_scan()
plot_bench_zvec_synth()
plot_bench_zvec_encode()
plot_bench_zvec_decode()
