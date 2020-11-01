FROM teeks99/clang-ubuntu:10

RUN apt-get update && \
	apt-get -y install software-properties-common
RUN apt-add-repository ppa:yubico/stable
RUN apt-get update && \
	apt-get -y install \
		pkg-config \
		curl \
		git \
		m4 \
		shellcheck \
		clang-tidy-10 \
		libfido2-dev \
		libssl-dev \
		libcbor-dev \
		libsodium-dev
# work around https://bugs.launchpad.net/ubuntu/+source/libcbor/+bug/1858835
RUN curl -o /usr/lib/x86_64-linux-gnu/pkgconfig/libcbor.pc https://gist.githubusercontent.com/mjec/ecc2f4421a6d3f2d798bedc23c5665b7/raw/ac817fd599285e9e1a483f72b1801ebb750213a6/libcbor.pc

RUN ln -s /usr/bin/clang-10 /usr/bin/clang && \
	ln -s /usr/bin/clang-format-10 /usr/bin/clang-format && \
	ln -s /usr/bin/clang-tidy-10 /usr/bin/clang-tidy

COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
