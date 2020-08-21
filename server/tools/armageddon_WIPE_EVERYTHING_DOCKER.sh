#!/bin/bash
# shellcheck disable=SC2046
#
# server/tools/armageddon_WIPE_EVERYTHING_DOCKER.sh
#
# https://stackoverflow.com/questions/34658836/docker-is-in-volume-in-use-but-there-arent-any-docker-containers

# set -e

confirm() {
    # call with a prompt string or use a default
    # https://stackoverflow.com/questions/3231804/in-bash-how-to-add-are-you-sure-y-n-to-any-command-or-alias
    read -r -p "${1:-Are you sure? [y/N]} " response
    case "$response" in
        [yY][eE][sS]|[yY])
            true
            ;;
        *)
            false
            ;;
    esac
}

removecontainers() {
    echo "- Stopping all Docker containers..."
    docker stop $(docker ps -aq)
    echo "- Deleting all Docker containers..."
    docker rm $(docker ps -aq)
}

armageddon() {
    echo "- Proceeding to Docker armageddon."
    removecontainers
    echo "- Pruning all networks..."
    docker network prune -f
    echo "- Deleting all dangling images..."
    docker rmi -f $(docker images --filter dangling=true -qa)
    echo "- Deleting all volumes..."
    docker volume rm $(docker volume ls --filter dangling=true -q)
    echo "- Deleting all images..."
    docker rmi -f $(docker images -qa)
}

confirm "Wipe EVERY aspect of Docker data on this computer?" || exit 1
confirm "This includes data volumes. Are you sure?" || exit 1
confirm "Last chance -- you may lose databases. Really sure?" || exit 1

armageddon
echo "- Done."
