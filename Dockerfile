FROM jianann/alpine-clang as build
COPY . /in3/
WORKDIR /in3/
USER root
RUN cd /in3/ && rm -rf build; 
RUN cd /in3/ && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=MINSIZEREL -DIN3_SERVER=true -DUSE_CURL=false .. && make in3


FROM alpine:latest
COPY --from=build /in3/build/bin/in3 /bin/in3
EXPOSE 8545
ENTRYPOINT ["/bin/in3"]
CMD ["--help"]