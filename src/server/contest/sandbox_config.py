from pathlib import Path

cwd = Path.cwd()
SANDBOX_PATH = cwd / "contest" / "sandbox"

EXE = SANDBOX_PATH / "sandbox-exe"
MEMORY_LIMIT = "1M"
TIME_LIMIT = "1000000000"  # in nano( 10^-9 ) seconds
MAX_PIDS = "4"
MEMORY_CGROUP = "/sys/fs/cgroup/memory/test"
CPUACCT_CGROUP = "/sys/fs/cgroup/cpuacct/test/"
PIDS_CGROUP = "/sys/fs/cgroup/pids/test/"
JAIL_DIR = SANDBOX_PATH / "jail"
# EXECUTABLE_FILE = "executable"
INPUT_FILE = ""
# OUTPUT_FILE = SANDBOX_PATH / "output"
WHITELIST = SANDBOX_PATH / "wl"  # wl for sys calls
UID = "1000"
GID = "1000"
