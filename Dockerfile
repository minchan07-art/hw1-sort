# syntax=docker/dockerfile:1
# Lab container with only what is needed to build and run the code.
#
# Sources are not baked into the image: compose.yml bind-mounts the repository
# at /work, so code is edited on the host and only run here.
FROM debian:trixie-slim

# build-essential: gcc, make and the C standard library headers
# gdb: debugger
# python3: draws the charts (tools/plot.py, standard library only)
# git, less: to work with the repository from a Codespaces terminal
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential gdb python3 git less ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
