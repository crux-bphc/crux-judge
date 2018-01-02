# Directory Structure

This doc introduces the directory structure of the project and broadly introduces the functions of modules. The default file names of django have been maintained. Contributors and maintainers are requested to keep this updated.

### /src/server/
```
.
├── src
│   └── server
│       ├── add_student_records.py
│       ├── contest
│       ├── judge
│       ├── manage.py
│       ├── prepare_cgroups
│       ├── student_records.csv
│       └── bank
```
This is the main directory of the project. `/server/` contains the django project called "judge". `judge` currently contains two "apps" : `/contest` and `/bank`.

`bank` app provides model for problem bank table in the database. It contains a `/testcase` directory which contains all the testcases of problems in problem bank. Also provides various views used in admin app.

`contest` app is reponsible for running the contest. It contains submissions, models, [sandbox](https://github.com/ajay0/sandbox). Refer to [contest doc](contest.md) for a more detailed documentation of the `contest` app.

`prepare_cgroups` script is run everytime the server is hosted to create cgroup directories for sandbox. This has been tested on Ubuntu wherein the default cgroup directories is `/sys/fs/cgroup/`. So this script may need to be tweaked if the mentioned directory is not your OS's default cgroup directory.

`add_student_records.py` is an experimental script still under development to enable contest hosts to add students via `student_records.csv` CSV. Feel free to change the script and csv file as per requirements.


### /docs/
```
.
├── docs
│   ├── admin.md
│   ├── contest.md
│   ├── install.md
│   ├── run.md
│   └── structure.md
```
This directory contains this doc and other documentation of the project. Try to keep these docs as updated as possible.

### /tools/
```
.
├── tools
    ├── activate
    ├── setup
    ├── setup_db
    └── setup_django
```
Contains various scripts to assist setting up the environment for the project.
