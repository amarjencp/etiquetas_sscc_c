# Use a minimal Debian-based image
FROM debian:bookworm-slim

# Install build tools and dependencies
RUN apt-get update && \
    apt-get install -y build-essential libhpdf-dev libzint-dev libpng-dev

COPY /etiquetas_sscc_c /workspace/source

# Set the working directory
WORKDIR /workspace/source

# Compile the program
RUN gcc etiquetas_sscc.c -o etiquetas -lzint -lhpdf -lpng -lz -Wall -O3 -march=native -mtune=native

# Default command to run when container starts
CMD ./etiquetas /workspace/input/test.csv && mv /workspace/source/sscc_labels.pdf /workspace/output/
