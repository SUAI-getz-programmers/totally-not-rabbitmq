FROM build AS build

# Build the client executable
RUN cd build && \
    make -j$(nproc) rabbitClientL3cmd

# Runtime stage
FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    libboost-all-dev

# Copy the built executable from the build stage
COPY --from=build /usr/src/rabbit/build/rabbitClientL3cmd /usr/local/bin/

# Run the client
CMD ["rabbitClientL3cmd", "-i", "2", "-h", "server", "-p", "3030"]