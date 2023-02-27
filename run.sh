set -ex

docker run -it --rm \
 -v `realpath .`:/tarantool \
 ubuntu:20.04 \
 bash -x -c \
 "sed -i -e 's|# deb-src|deb-src|g' /etc/apt/sources.list
  apt update
  DEBIAN_FRONTEND=noninteractive apt-get build-dep -y tarantool
  DEBIAN_FRONTEND=noninteractive apt-get install -y zlib1g-dev python3-gevent python3-yaml git
  cd /tarantool/tarantool-1.10.15.0.e14d0b4eab
  dpkg-buildpackage -b --no-sign"