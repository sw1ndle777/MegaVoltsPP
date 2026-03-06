#!/bin/bash

# Usage: ./rebuild.sh [env|service] [service]

ARG1=$1
ARG2=$2

ENV="prod"
SERVICE=""

if [ "$ARG1" == "dev" ] || [ "$ARG1" == "prod" ]; then
    ENV=$ARG1
    SERVICE=$ARG2
elif [ -n "$ARG1" ]; then
    ENV="prod"
    SERVICE=$ARG1
else
    ENV="prod"
    SERVICE=""
fi

if [ "$ENV" == "dev" ]; then
    COMPOSE_FILE="docker-compose.dev.yml"
else
    if [ -f "docker-compose.prod.yml" ]; then
        COMPOSE_FILE="docker-compose.prod.yml"
    else
        echo "Warning: Could not find docker-compose.prod.yml. Using docker-compose.yml"
        COMPOSE_FILE="docker-compose.yml"
    fi
fi

OS_NAME="$(uname -s)"
case "$OS_NAME" in
    Linux*)     SUDO="sudo" ;;
    CYGWIN*|MINGW*|MSYS*) SUDO="" ;; # Windows (Git Bash)
    *)          SUDO="" ;;
esac

echo "------------------------------------------------"
echo "Environment : $ENV"
echo "Service     : ${SERVICE:-[ALL]}"
echo "File        : $COMPOSE_FILE"
echo "OS          : $OS_NAME"
echo "------------------------------------------------"

echo "Building..."
$SUDO docker compose -f "$COMPOSE_FILE" build $SERVICE --no-cache --parallel --force-rm

if [ $? -ne 0 ]; then
    echo "Build failed! Aborting."
    exit 1
fi

echo "Starting up..."
$SUDO docker compose -f "$COMPOSE_FILE" up $SERVICE -d --force-recreate --remove-orphans