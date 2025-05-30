# Use an official base image
FROM ubuntu:22.04

# Set environment variables to prevent some interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools, valgrind, Python, and pip
RUN apt-get update && \
    apt-get install -y \
    clang \
    make \
    valgrind \
    python3 \
    python3-pip && \
    pip3 install pytest && \
    apt-get clean

# Set clang as the default compiler
ENV CC=clang
ENV CXX=clang++

# Create app directory
WORKDIR /app

# Copy source and Makefile
COPY src/ ./src/
COPY tests/ ./tests/
COPY Makefile .

WORKDIR /app

# Build the project at image build time
RUN make rebuild

# Expose the server port
EXPOSE 8124

# Run the server by default
CMD ["/bin/sh", "-c", "exec stdbuf -oL ./fwc"]
