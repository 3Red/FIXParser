#!/usr/bin/env python
import glob
import sys

def normalize(filename_in, filename_out):
    times = open(filename_in).readlines()
    times = [float(i) for i in times]
    times.sort()
    total = sum(times)
    running_percent = 0.0
    output = open(filename_out, "w")
    for t in times:
        running_percent += (t/total)
        output.write("{0} {1}\n".format(t, running_percent*100.0))

def main():
    output_filenames = []
    for i in sorted(glob.glob("times-?.txt")):
        output_filenames.append(i.replace(".txt", "-normal.txt"))
        normalize(i, output_filenames[-1])

    o = open("plot.gnu", "w")
    o.write("""
set termoption dashed
do for [i=1:10] {
    set style line i linewidth 2
}
set style increment userstyles
set style line 6
set style line 6 linecolor "darkgray"
set xlabel "nanoseconds
set xrange [0:50000]
set yrange [0:100]
set ylabel "%"
plot """)
    o.write(",".join(['"{0}" with lines'.format(i) for i in output_filenames]))
    o.write('\n')
    o.write('pause -1\n')


if __name__ == "__main__":
    main()
