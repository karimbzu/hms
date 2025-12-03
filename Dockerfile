# ---- Stage 1: Builder ----
FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    libasio-dev libsqlite3-dev \
    curl wget && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source files
COPY . .

# Clone Crow headers (older versions require both crow.h and crow folder)
RUN mkdir -p include && \
    if [ ! -f include/crow.h ]; then \
        git clone https://github.com/CrowCpp/crow.git /tmp/crow && \
        cp /tmp/crow/include/crow.h include/ && \
        cp -r /tmp/crow/include/crow include/; \
    fi

# Build using CMake
RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)


# ---- Stage 2: Runtime ----
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libsqlite3-0 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled binary from builder
COPY --from=builder /app/build/crow_sqlite_crud /app/crow_sqlite_crud

# Persistent DB folder
RUN mkdir -p /app/data

EXPOSE 3000

CMD ["./crow_sqlite_crud"]
