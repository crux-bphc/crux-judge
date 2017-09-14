# Running Instructions

## Developers

0. Follow the [install](docs/install.md) instructions

1. To start the Django Local Server, run the following commands:

```bash
cd ~/crux-judge
source env/bin/activate
cd src/server
sudo python3 manage.py runserver
```

Check if the localserver is working, by opening your browser and typing `127.0.0.1:8000`
If you see the contest login page, you have successfully run the server!
