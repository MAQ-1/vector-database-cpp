# ============================================
# ALL-IN-ONE CONTAINER
# Backend + Frontend + Caddy in one image
# ============================================

# Stage 1: Build React Frontend
FROM node:20-alpine AS frontend-builder

WORKDIR /app/frontend

# Copy package files
COPY frontend/package*.json ./

# Install dependencies with increased timeout and retry
RUN npm config set fetch-retry-maxtimeout 120000 && \
    npm config set fetch-retry-mintimeout 20000 && \
    npm ci --production=false --prefer-offline --no-audit

# Copy source code
COPY frontend/ ./

# Build for production
RUN npm run build

# Stage 2: Build C++ Backend
FROM ubuntu:22.04 AS backend-builder

ENV DEBIAN_FRONTEND=noninteractive
ENV DEBIAN_PRIORITY=critical

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    cmake \
    libpoppler-cpp-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/include/ ./include/
COPY backend/external/ ./external/
COPY backend/src/ ./src/
COPY backend/main.cpp ./

RUN g++ -std=c++17 -O2 \
    -I include \
    -I external \
    main.cpp \
    src/VectorDatabase.cpp \
    src/VectorRecord.cpp \
    src/KDTree.cpp \
    src/HNSW.cpp \
    src/LSH.cpp \
    src/similarity.cpp \
    src/OllamaClient.cpp \
    src/DocumentIngestion.cpp \
    src/PdfExtractor.cpp \
    -lpoppler-cpp \
    -o VectorDB

# Stage 3: Runtime with Caddy
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV DEBIAN_PRIORITY=critical

# Install Caddy and runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    debian-keyring \
    debian-archive-keyring \
    apt-transport-https \
    curl \
    ca-certificates \
    libpoppler-cpp0v5 \
    bash \
    gnupg \
    && curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg \
    && curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | tee /etc/apt/sources.list.d/caddy-stable.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends caddy \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -s /bin/bash appuser

WORKDIR /app

# Copy compiled backend
COPY --from=backend-builder /app/VectorDB /app/VectorDB

# Copy frontend build to Caddy serve directory
COPY --from=frontend-builder /app/frontend/dist /srv

# Copy production Caddyfile
COPY Caddyfile.production /etc/caddy/Caddyfile

# Set permissions
RUN chown -R appuser:appuser /app && \
    mkdir -p /app/data && \
    chown -R appuser:appuser /app/data

# Create startup script to run both backend and Caddy
RUN echo '#!/bin/bash' > /start.sh && \
    echo 'set -e' >> /start.sh && \
    echo 'echo "Starting VectorDB Backend..."' >> /start.sh && \
    echo 'su -c "cd /app && ./VectorDB &" appuser' >> /start.sh && \
    echo 'echo "Starting Caddy..."' >> /start.sh && \
    echo 'caddy run --config /etc/caddy/Caddyfile --adapter caddyfile' >> /start.sh && \
    chmod +x /start.sh

# Expose ports
EXPOSE 80 443 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=15s --retries=3 \
    CMD curl -f http://localhost/api/status || exit 1

CMD ["/start.sh"]