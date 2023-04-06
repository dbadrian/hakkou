ARG USER=dev
ARG UID=1000
ARG GID=${UID}

FROM espressif/idf:latest
cd 
ARG USER
ARG UID
ARG GID

# Create the user
RUN addgroup --gid ${GID} "${USER}"
RUN adduser --home /home/$USER --disabled-password --gecos "" --uid ${UID} --ingroup "${USER}" "${USER}" 
RUN usermod --append --groups "dialout,plugdev" "${USER}"


RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc
RUN echo "source /opt/esp/idf/export.sh" >> /home/${USER}/.bashrc

RUN apt-get update \
    && apt-get install -y -q \
    locales


RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
