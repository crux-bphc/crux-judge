# Running Instructions

## Developers

0. Follow the [install](docs/install.md) instructions

1. To start the Django Local Server, run the following commands:

```bash
cd ~/crux-judge
source env/bin/activate
python3 judge/manage.py runserver
```

# Testing Instructions

## Developers

* [Py file for compile+execute function, step1.py](judge/trial/step1.py)
Note that [step1.py](judge/trial/step1.py) is a stand-alone file, unrelated to django
It just contains the handler function, nothing more (move it according to project structure, if need be)

To test the working of the file,
+ create a [hello_world.c](judge/trial/step1.py) test file with `gedit judge/trial/hello_world.c` 
+ open test1.py with `gedit judge/trial/step1.py` to see the handler function
+ to test the [file](judge/trial/step1.py), run the following command:
`python3 judge/trial/step1.py`
