FROM build AS build

# Build the worker executable
RUN cd build && \
    make -j$(nproc) rabbitWorkerL3cmd

# Runtime stage
FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libboost-all-dev

# Copy the built executable from the build stage
COPY --from=build /usr/src/rabbit/build/rabbitWorkerL3cmd /usr/local/bin/

# Run the worker
CMD ["rabbitWorkerL3cmd", "-i", "123", "-h", "server", "-p", "3030", "-c", "1000"]