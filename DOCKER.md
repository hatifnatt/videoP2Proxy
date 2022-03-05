# Running in Docker

## Build

Clone repository, change directory to freshly cloned repo

```sh
git clone https://github.com/hatifnatt/videoP2Proxy.git
# if Dockerfile not present in master branch, switch to docker-build branch
# git checkout docker-build
cd videop2proxy
docker build -t videop2proxy --target run-env .
```

## Use

When running interactively, to be able to stop container with `Ctrl-C` run with `--init`

```sh
docker run --rm --init --name videop2proxy videop2proxy --cam-id LAYDLWRCR5U953N9111A --cam-pass 11111111
```

When runnig in background (daemon) mode, `--init` is not required

```sh
docker run --rm -d --name videop2proxy videop2proxy --cam-id LAYDLWRCR5U953N9111A --cam-pass 11111111
```

### Stop container running in background

```sh
docker stop videop2proxy
```
