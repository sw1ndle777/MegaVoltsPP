#!/bin/bash

# Usage: ./shutdown.sh [env|service] [service]

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
    Darwin*)    SUDO="" ;;
    CYGWIN*|MINGW*|MSYS*) SUDO="" ;;
    *)          SUDO="" ;;
esac

echo "------------------------------------------------"
echo "Action      : SHUTDOWN"
echo "Environment : $ENV"
echo "Service     : ${SERVICE:-[ALL]}"
echo "File        : $COMPOSE_FILE"
echo "------------------------------------------------"

if [ -z "$SERVICE" ]; then
    echo "Stopping and removing all containers/networks..."
    $SUDO docker compose -f "$COMPOSE_FILE" down --remove-orphans
else
    echo "Stopping