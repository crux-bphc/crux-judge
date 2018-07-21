# Docker

To run Crux Judge from a docker container, first install docker on your machine. 
Refer to official docker [installation docs](https://docs.docker.com/install/) to set it up.
Once docker is set up, you can build an image `cruxjudge` from this repository by running the following command 
```bash
docker build -t cruxjudge https://github.com/crux-bphc/crux-judge.git -f docker/Dockerfile
```
This will result in a `cruxjudge` image of size 524MB(at the time writing this).

To start a container `cruxjudge`, run the [docker_run](/docker/docker_run.sh) script after adding relevant parameters to it. 