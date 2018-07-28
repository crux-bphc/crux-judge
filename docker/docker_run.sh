#!/bin/bash

# Set the following parameters
# NOTE: PATHS MUST BE ABSOLUTE.
PORT=80
DOCKER_IMAGE_ID=""
TESTCASES_DIR=""
SUBMISSIONS_DIR=""
PROBLEM_FILES_DIR=""
DB_PATH=""

# Create directories and files, if do not exist already.
mkdir -pv $TESTCASES_DIR $SUBMISSIONS_DIR $PROBLEM_FILES_DIR
touch $DB_PATH

# Flags required for `docker run`
# Priviliged flag is required to access container cgroups for sandbox
# The server is will start at port number $PORT on the host machine.
# 4 bind mounts are made to keep the following data persistent on the host machine:
#   1. Testcases directory:     To testcases of problems in problem bank
#   2. Submissions directory:   To store best and latest submissions of users
#   3. Problem files directory: To store PDF files of problem statements 
#   4. Database:                SQLite3 database file
flags=(
    -t
    --privileged
    -p $PORT:8000
    --name cruxjudge
    --mount type=bind,source=$TESTCASES_DIR,target="/root/home/cruxjudge/src/server/bank/testcases/" 
    --mount type=bind,source=$SUBMISSIONS_DIR,target="/root/home/cruxjudge/src/server/contest/submissions/" 
    --mount type=bind,source=$PROBLEM_FILES_DIR,target="/root/home/cruxjudge/src/server/bank/problem_files/"
    --mount type=bind,source=$DB_PATH,target="/root/home/cruxjudge/src/server/db.sqlite3" 
    --rm
)

# Add custom flags, if any
for var in "$@"
do
    case $var in

    # `-i` flag. Opens docker container in interactive mode. Useful for first time set-up.
    "-i")  flags+=(-i --entrypoint="/bin/bash")
            ;;
    
    esac
done

# Run container with the above flags
docker run "${flags[@]}" $DOCKER_IMAGE_ID

