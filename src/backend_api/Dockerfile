###########
# BUILDER #
###########

# pull official base image
FROM python:3.12 as builder

# set work directory
WORKDIR /usr/src/app

# set environment variables
ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

# install system dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends gcc

# install python dependencies
COPY ./src/pixel_server/requirements.txt .
RUN pip wheel --no-cache-dir --no-deps --wheel-dir /usr/src/app/wheels -r requirements.txt

FROM python:3.12-slim

WORKDIR /home/backend

# set environment variables
ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

ARG GIT_COMMIT_HASH
ENV GIT_COMMIT_HASH $GIT_COMMIT_HASH

COPY ./src/pixel_server .
COPY ./assets/pixel.png .

# install dependencies
COPY --from=builder /usr/src/app/wheels /wheels
COPY --from=builder /usr/src/app/requirements.txt .
RUN pip install --upgrade pip
RUN pip install --no-cache /wheels/*

# For non-builder install
# RUN pip3 install -r ./requirements.txt

CMD python ./pixel_server.py
