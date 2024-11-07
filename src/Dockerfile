# Use an Ubuntu base image with support for C++ and Boost
FROM ubuntu:20.04

# Set non-interactive environment variable
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages and dependencies
RUN ln -fs /usr/share/zoneinfo/Etc/UTC /etc/localtime && \
    apt-get update && \
    apt-get install -y \
        tzdata \
        g++ \
        cmake \
        libboost-all-dev \
        libssl-dev \
        wget && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory in the container
WORKDIR /app

# Copy the C++ source code into the container
COPY ./src/smart_mailer.cpp .

# Copy the assets directory into the container
COPY ./src/assets /app/assets

# Compile the C++ code
RUN g++ smart_mailer.cpp -o smart_mailer -lboost_system -lssl -lcrypto -lpthread

# Define environment variables for the SMTP configuration
ENV SMTP_SERVER=smtp.example.com
ENV SMTP_DOMAIN=example.com
ENV EMAIL=your_email@example.com
ENV PASSWORD=your_password

# Run the application
CMD ["./smart_mailer"]