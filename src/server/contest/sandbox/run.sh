gcc *.c -lm -pthread -lseccomp
sudo ./a.out "1M" "1000000000" "4" "/sys/fs/cgroup/memory/test/" "/sys/fs/cgroup/cpuacct/test/" "/sys/fs/cgroup/pids/test/" ./jail/ executable ./input ./jail/output ./wl "1000" "1000"
