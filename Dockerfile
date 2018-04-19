FROM greyltc/archlinux-aur
MAINTAINER Bernhard Specht <bernhard@specht.net>

RUN pacman -Syu --noconfirm --needed
RUN pacman -S cmake --noconfirm --needed
RUN pacman -S rapidjson --noconfirm --needed
RUN pacman -S curl --noconfirm --needed
RUN pacman -S boost --noconfirm --needed
RUN pacman -S openssh --noconfirm --needed

RUN su docker -c 'gpg --recv-keys 8C718D3B5072E1F5'
RUN su docker -c 'pacaur -S --noprogressbar --noedit --noconfirm mysql-connector-c++ --needed'
RUN su docker -c 'pacaur -S --noprogressbar --noedit --noconfirm grpc --needed'
RUN su docker -c 'pacaur -S --noprogressbar --noedit --noconfirm google-glog --needed'

WORKDIR /srv/app/cpp-burst-pool-node
COPY src src
COPY CMakeLists.txt .
COPY Makefile .
COPY protos protos
RUN mkdir -p build
RUN make

COPY config.json .

EXPOSE 8126

CMD ["build/src/server"]
