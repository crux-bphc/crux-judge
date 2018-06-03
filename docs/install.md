# Installation Instructions

## Developers

Clone the repository from [GitHub](https://github.com/crux-bphc/crux-judge).

You need pip3, MySQL and virtualenv installed on your system. For Debian/Ubuntu, run the following commands:

```bash
sudo apt install python3-pip -y
sudo pip3 install virtualenv
sudo apt install postgresql-9.6
```

Now, run the following commands to install the virtual environment.

```bash
git clone -b development https://github.com/crux-bphc/crux-judge
cd crux-judge
tools/setup
```
Now, run the following command to compile your own sandbox-exe:

```bash
sudo apt install libseccomp2 libseccomp-dev
gcc src/server/contest/sandbox/*.c -lm -pthread -lseccomp -o src/server/contest/sandbox/sandbox-exe
```


<!-- TODO : Add instructions for setting up postgresql-9.6 -->

Now, you should have virtual-env installed, as well as dependencies installed/updated using pip3. You need to activate the virtual environment just installed to start developing.

```bash
source tools/activate
```

To test if the Virtual Environment has been setup, run `which python3`. If it shows the path to your Project Directory, then you are now ready to develop.

Lastly, we need to set a few things for Django. Run the following command :
```bash
tools/setup_django
```
