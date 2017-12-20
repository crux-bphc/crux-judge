import os

EXE = os.getcwd() + "/contest/sandbox/sandbox-exe"
MEMORY_LIMIT = "1M"
TIME_LIMIT = "1000000000" #in nano( 10^-9 ) seconds
MAX_PIDS = "4"
MEMORY_CGROUP = "/sys/fs/cgroup/memory/test"
CPUACCT_CGROUP = "/sys/fs/cgroup/cpuacct/test/"
PIDS_CGROUP = "/sys/fs/cgroup/pids/test/"
JAIL_DIR = os.getcwd() + "/contest/sandbox/jail/"
# EXECUTABLE_FILE = "executable"
INPUT_FILE = ""
# OUTPUT_FILE = os.getcwd() + "/contest/sandbox/output"
WHITELIST = os.getcwd() + "/contest/sandbox/wl" #wl for sys calls
UID = "1000"
GID = "1000"
