#!/bin/bash
# shellcheck disable=SC2046
#
# server/tools/cataclysm_WIPE_DOCKER_EXCEPT_VOLUMES.sh

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

cataclysm() {
    removecontainers
    # echo "- Deleting all networks..."
    # docker network rm $(docker network ls -q)
    echo "- Pruning all networks..."
    docker network prune -f
    echo "- Deleting all images..."
    docker rmi -f $(docker images -qa)
}

confirm "Wipe Docker data (images, containers, networks) EXCEPT volumes?" || exit 1

cataclysm
echo "- Done."
