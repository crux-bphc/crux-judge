cd# Running Instructions

## Developers

0. Follow the [install](docs/install.md) instructions

1. To start the Django Local Server, run the following commands:

```bash
cd ~/crux-judge
source env/bin/activate
python3 judge/manage.py runserver
```

Check if the localserver is working, by opening your browser and typing `127.0.0.1:8000`
If you see the Django Congratulations message, you have successfully run the server!

2. To test file uploading, go to `127.0.0.1:8000/trial/`
Select a file to upload, then click on submit to upload the file
To confirm a successful upload, check in `crux-judge/src/server/submissions`.

If you see your file present, congrats! You've successfully uploaded a file to the server!
If not, PLEASE open an issue on the GitHub repo!

# Testing/Tweaking Instructions

## Developers

* [Py file for compile+execute function, step1.py](judge/trial/step1.py)
Note that [step1.py](judge/trial/step1.py) is a stand-alone file, unrelated to django
It just contains the handler function, nothing more (move it according to project structure, if need be)

To test the working of the file,
+ create a [hello_world.c](judge/trial/step1.py) test file with `gedit judge/trial/hello_world.c`
+ open test1.py with `gedit judge/trial/step1.py` to see the handler function
+ to test the [file](judge/trial/step1.py), run the following command:
`python3 judge/trial/step1.py`
+ the file takes input(if required) from a file 'input' located in the same directory as the C file

* [add_student_records.py](srv/server/add_student_records.py) takes csv records of the form "username,email,name,password"
    from file [student_records.csv](src/server/student_records.csv) and adds to the database

## Contest Testing

* Follow tools/install
* Run /src/server/django_setup to create a django migrations and superuser
* Run the following command in /src/server/ to start the server:
`python3 manage.py runserver`
* The server will run on `127.0.0.1:8000` by default. Visit `127.0.0.1:8000/admin/` to manage databases.
* Add Problems in "Problems". Add users.
* Add problems to "Contest Problems" to add problems to the contest.
* Add testcases to the folder /src/server/contest/testcases/ in the following format.
    /testcases/ directory should contain folders whose name are the problem_id of the problem.
    The problem folder must contain equal number of input and output files. Files must be of the format inputX and outputX, where X belongs to integers
    For example :
        /testcases/2/ must contain testcases data for problem with problem_id = 2
        Suppose there are two testcases for the problem, create 4 files named : input1,input2,output1,outpu2
        outputX corresponds to the output for the case when input is inputX. Add as many cases as desired.
* Visit `127.0.0.1:8000/contest/` to access contest        
