# Installation Instructions

## Developers

Clone the repository from [GitHub](https://github.com/CRUx-BPHC/crux-judge).

You need pip3 and virtualenv installed on your system. For Debian/Ubuntu, run the following commands:

```bash
sudo apt install python3-pip -y
sudo pip3 install virtualenv
```

Now, run the following commands to install the virtual environment.

```bash
git clone https://github.com/CRUx-BPHC/crux-judge.git
cd crux-judge
tools/setup.sh
```

Now, you should have virtual-env installed, as well as dependencies installed/updated using pip3. You need to activate the virtual environment just installed to start developing.

```bash
source env/bin/activate
```

To test if the Virtual Environment has been setup, run `which python3`. If it shows the path to your Project Directory, then you are now ready to develop.