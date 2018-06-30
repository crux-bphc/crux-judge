FROM ubuntu:18.04
ADD / /root/home/cruxjudge/
LABEL maintainer="Basu Dubey <basu.96@gmail.com>"

# Install dependencies
RUN apt-get update && apt-get install -y \
    git \
    sudo \
    libseccomp2 \
    python3 \
    python3-pip \
    nginx \
    nano \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install Django==1.11.2 uwsgi django-ipware psycopg2 \
    && mkdir -p /root/home/cruxjudge/src/server/contest/testcases \
    && mkdir -p /root/home/cruxjudge/src/server/contest/sandbox/jail

# Add nginx config
ADD cruxjudge_nginx.conf /etc/nginx/sites-available/
RUN ln -s /etc/nginx/sites-available/cruxjudge_nginx.conf /etc/nginx/sites-enabled/cruxjudge_nginx.conf \
    && chown -R www-data /root

WORKDIR /root/home/cruxjudge/src/server

CMD python3 manage.py collectstatic \
    && python3 manage.py migrate \
    && service nginx restart \
    && uwsgi --ini /root/home/cruxjudge/cruxjudge_uwsgi.ini

EXPOSE 8000
