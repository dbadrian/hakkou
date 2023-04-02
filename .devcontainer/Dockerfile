FROM espressif/idf:latest
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc

RUN apt-get update \
    && apt-get install -y -q \
    locales


RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8