FROM ubuntu:18.04
ADD / /root/home/cruxjudge/
LABEL maintainer="Basu Dubey <basu.96@gmail.com>"

RUN ["apt", "update"]
RUN ["apt", "install", "-y", "apt-utils"]
RUN ["apt", "install", "-y", "git", "sudo", "libseccomp2", "python3", "python3-pip", "nginx", "nano"]
RUN ["pip3", "install", "Django==1.11.2", "uwsgi", "django-ipware", "psycopg2"]
COPY cruxjudge_nginx.conf /etc/nginx/sites-available/
RUN ["ln", "-s", "/etc/nginx/sites-available/cruxjudge_nginx.conf", "/etc/nginx/sites-enabled/cruxjudge_nginx.conf"]
RUN ["chown", "-R", "www-data", "/root"]

WORKDIR /root/home/cruxjudge/src/server
CMD ["python3", "manage.py", "collectstatic"]
CMD ["python3", "manage.py", "migrate"]
CMD ["service", "nginx", "restart"]
CMD ["uwsgi", "--ini", "/root/home/cruxjudge/cruxjudge_uwsgi.ini"]

EXPOSE 8000
