#!/usr/bin/env python3
import matplotlib.pyplot as plt
import json
import sys

def load_data(filename):
    timestamps = []
    out1, out2, out3 = [], [], []

    last1 = last2 = last3 = None

    with open(filename, "r") as f:
        for line in f:
            # Try to load JSON object
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                # Línea no válida → ignorar
                continue

            timestamps.append(obj["timestamp"])

            # --- OUT1 ---
            if obj["out1"] != "--":
                last1 = float(obj["out1"])
            out1.append(last1)

            # --- OUT2 ---
            if obj["out2"] != "--":
                last2 = float(obj["out2"])
            out2.append(last2)

            # --- OUT3 ---
            if obj["out3"] != "--":
                last3 = float(obj["out3"])
            out3.append(last3)

    return timestamps, out1, out2, out3


def plot_graph(timestamps, out1, out2, out3):
    fig, axes = plt.subplots(3, 1, sharex=True, figsize=(12, 8))

    axes[0].plot(timestamps, out1)
    axes[0].set_ylabel("out1")

    axes[1].plot(timestamps, out2)
    axes[1].set_ylabel("out2")

    axes[2].plot(timestamps, out3)
    axes[2].set_ylabel("out3")
    axes[2].set_xlabel("timestamp (ms)")

    plt.tight_layout()
    #plt.show()
    plt.savefig("graph.png")


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <data_file_name>")
        sys.exit(1)

    filename = sys.argv[1]
    timestamps, out1, out2, out3 = load_data(filename)
    plot_graph(timestamps, out1, out2, out3)


if __name__ == "__main__":
    main()
