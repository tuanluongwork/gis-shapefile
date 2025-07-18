# Docker Configuration for GIS Shapefile Processor

# Multi-stage build for efficient container size
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgtest-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create application directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc)

# Production stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 gisuser

# Create application directory
WORKDIR /app

# Copy built binaries from builder stage
COPY --from=builder /app/build/bin/* /usr/local/bin/
COPY --from=builder /app/data /app/data

# Set ownership
RUN chown -R gisuser:gisuser /app
USER gisuser

# Expose port for web server
EXPOSE 8080

# Default command runs the web server
CMD ["gis-server", "8080"]

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Labels for container metadata
LABEL org.opencontainers.image.title="GIS Shapefile Processor"
LABEL org.opencontainers.image.description="Modern C++ GIS library for shapefile processing and geocoding"
LABEL org.opencontainers.image.version="1.0.0"
LABEL org.opencontainers.image.vendor="tuanluongwork"
LABEL org.opencontainers.image.source="https://github.com/tuanluongwork/ShapeFile"
