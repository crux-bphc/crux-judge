# Installation Instructions

## Developers

Clone the repository from [GitHub](https://github.com/CRUx-BPHC/crux-judge).

You need pip3, MySQL and virtualenv installed on your system. For Debian/Ubuntu, run the following commands:

```bash
sudo apt install python3-pip -y
sudo pip3 install virtualenv
sudo apt install mysql-server mysql-client
```

Now, run the following commands to install the virtual environment.

```bash
git clone -b contest https://github.com/basu96/crux-judge.git
cd crux-judge
tools/setup
```

Now, you should have virtual-env installed, as well as dependencies installed/updated using pip3. You need to activate the virtual environment just installed to start developing.

```bash
source tools/activate
```

To test if the Virtual Environment has been setup, run `which python3`. If it shows the path to your Project Directory, then you are now ready to develop.

We'll also create a new user on mysql and create a database on it. Run the following command and enter the root password for mysql when prompted :
```bash
tools/setup_db
```
