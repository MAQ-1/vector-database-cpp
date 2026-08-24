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
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/include/ ./include/
COPY backend/external/ ./external/
COPY backend/src/ ./src/
COPY backend/main.cpp ./

RUN g++ -std=c++17 -O2 \
    -DCPPHTTPLIB_OPENSSL_SUPPORT \
    -I include \
    -I external \
    -I /usr/include/poppler/cpp \
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
    -lssl \
    -lcrypto \
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
    libssl3 \
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
    echo '' >> /start.sh && \
    echo '# Print environment (without secrets)' >> /start.sh && \
    echo 'echo "=== Environment Check ==="' >> /start.sh && \
    echo 'echo "AICREDITS_BASE_URL: ${AICREDITS_BASE_URL}"' >> /start.sh && \
    echo 'echo "AICREDITS_MODEL: ${AICREDITS_MODEL}"' >> /start.sh && \
    echo 'echo "AICREDITS_EMBEDDING_MODEL: ${AICREDITS_EMBEDDING_MODEL}"' >> /start.sh && \
    echo 'echo "API_KEY Set: $([ -n \"$AICREDITS_API_KEY\" ] && echo \"YES\" || echo \"NO\")"' >> /start.sh && \
    echo 'echo ""' >> /start.sh && \
    echo '' >> /start.sh && \
    echo '# Start backend' >> /start.sh && \
    echo 'echo "=== Starting VectorDB Backend on port 8080 ==="' >> /start.sh && \
    echo 'cd /app' >> /start.sh && \
    echo 'su -p -c "./VectorDB" appuser > /tmp/backend.log 2>&1 &' >> /start.sh && \
    echo 'BACKEND_PID=$!' >> /start.sh && \
    echo 'echo "Backend PID: $BACKEND_PID"' >> /start.sh && \
    echo '' >> /start.sh && \
    echo '# Wait for backend to start' >> /start.sh && \
    echo 'echo "Waiting for backend to start..."' >> /start.sh && \
    echo 'for i in {1..30}; do' >> /start.sh && \
    echo '    if curl -s http://localhost:8080/ > /dev/null 2>&1; then' >> /start.sh && \
    echo '        echo "Backend is UP!"' >> /start.sh && \
    echo '        break' >> /start.sh && \
    echo '    fi' >> /start.sh && \
    echo '    if ! kill -0 $BACKEND_PID 2>/dev/null; then' >> /start.sh && \
    echo '        echo "ERROR: Backend process died!"' >> /start.sh && \
    echo '        echo "=== Backend Logs ==="' >> /start.sh && \
    echo '        cat /tmp/backend.log' >> /start.sh && \
    echo '        exit 1' >> /start.sh && \
    echo '    fi' >> /start.sh && \
    echo '    echo "  Attempt $i/30..."' >> /start.sh && \
    echo '    sleep 1' >> /start.sh && \
    echo 'done' >> /start.sh && \
    echo '' >> /start.sh && \
    echo '# Tail backend logs in background' >> /start.sh && \
    echo 'tail -f /tmp/backend.log &' >> /start.sh && \
    echo '' >> /start.sh && \
    echo '# Start Caddy' >> /start.sh && \
    echo 'echo ""' >> /start.sh && \
    echo 'echo "=== Starting Caddy ==="' >> /start.sh && \
    echo 'exec caddy run --config /etc/caddy/Caddyfile --adapter caddyfile' >> /start.sh && \
    chmod +x /start.sh

# Expose ports
EXPOSE 80 443 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=15s --retries=3 \
    CMD curl -f http://localhost/api/status || exit 1

CMD ["/start.sh"]