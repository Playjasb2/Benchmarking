
import glob

# find all the comparison files
cmp_file_names = glob.glob("cmp_*.csv")

if cmp_file_names is not None:
    cmp_file_names.sort()

# open the base file
base_file = open("base.csv", "r")

# open the comparison files
cmp_files = []
for cmp_file_name in cmp_file_names:
    cmp_files.append(open(cmp_file_name, "r"))

# merge all the data into a single data file
data_kb = open("data_kb.csv", "w")
data_mb = open("data_mb.csv", "w")
for base_line in base_file:
    str = base_line.replace('\n', '')
    for cmp_file in cmp_files:
        x, y = cmp_file.readline().replace('\n', '').split(',')
        str += "," + y
    str += "\n"

    block_size = int(str.split(',')[0])
    if block_size <= 1024:
        data_kb.write(str)
    if block_size >= 1024:
        data_mb.write(str)

data_kb.close()
data_mb.close()
base_file.close()
for cmp_file in cmp_files:
    cmp_file.close()

# generate the plot file
def write_gp_file(f_name, f_input, f_output, title, x_label, y_label, x_start, x_end, x_offset, x_stride, legend_outside):
    gp_out = open(f_name, "w")
    gp_out.write("set terminal png\n")
    gp_out.write("set datafile separator \",\"\n")
    gp_out.write("set output '{}'\n".format(f_output))
    gp_out.write("set title '{}'\n".format(title))
    gp_out.write("set xlabel '{}'\n".format(x_label))
    gp_out.write("set ylabel '{}'\n".format(y_label))
    if legend_outside:
        gp_out.write("set key outside\n")
    gp_out.write("set xtics rotate\n")
    gp_out.write("set xrange [{}:{}]\n".format(x_start, x_end))
    gp_out.write("set xtics {},{}\n".format(x_offset, x_stride))
    gp_out.write("plot '{}' using 1:2 w lp title 'base', \\\n".format(f_input))

    for i in range(0, len(cmp_files)):
        gp_out.write("'' using 1:{} w lp title '{}', \\\n".format(3 + i, cmp_file_names[i]))

    gp_out.close()

write_gp_file("graph_kb.gp", "data_kb.csv", "shared_cache_kb.png", "Shared Cache Measurement (KB)", "Blocksize (KB)", "Miss Rate (%)", 256, 1024, 256, 64, False)
write_gp_file("graph_mb.gp", "data_mb.csv", "shared_cache_mb.png", "Shared Cache Measurement (MB)", "Blocksize (KB)", "Miss Rate (%)", 1024, 32 * 1024, 2048, 2018, True)
