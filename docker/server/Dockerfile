FROM ubuntu:latest AS build

# Install necessary packages
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    python3-venv \
    git \
    unzip \
    tree

# Create and activate a virtual environment
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

# Upgrade pip and install conan in the virtual environment
RUN pip install --upgrade pip
RUN pip install conan
RUN conan profile detect --force

WORKDIR /usr/src/rabbit

# Download source code
COPY . .

# RUN tree -L 4

# Install dependencies and build the project
RUN conan install . --output-folder=build --build=missing

RUN cd build && cmake ..

RUN cmake -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="conan_provider.cmake" -B build .

# Runtime stage
FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libboost-all-dev

# Copy the built executable from the build stage
COPY --from=build /usr/src/rabbit/build/rabbitServerL3cmd /usr/local/bin/

# Expose the server port
EXPOSE 3030

# Run the server
CMD ["rabbitServerL3cmd", "-p", "3030"]
