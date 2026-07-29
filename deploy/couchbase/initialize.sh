#!/usr/bin/env bash

set -euo pipefail

readonly couchbase_host="${HVN_COUCHBASE_HOST:-couchbase}"
readonly couchbase_cli="/opt/couchbase/bin/couchbase-cli"
readonly cbq="/opt/couchbase/bin/cbq"
readonly index_file="${HVN_COUCHBASE_INDEX_FILE:-/opt/haven/couchbase/indexes.sql}"

for variable_name in \
    HVN_COUCHBASE_USERNAME \
    HVN_COUCHBASE_PASSWORD \
    HVN_COUCHBASE_BUCKET \
    HVN_COUCHBASE_SCOPE; do
    if [[ -z "${!variable_name:-}" ]]; then
        echo "${variable_name} is required" >&2
        exit 1
    fi
done

readonly couchbase_name_pattern='^[A-Za-z0-9_%-]+$'
if [[ ! "${HVN_COUCHBASE_BUCKET}" =~ ${couchbase_name_pattern} ]]; then
    echo "HVN_COUCHBASE_BUCKET contains unsupported characters" >&2
    exit 1
fi
if [[ ! "${HVN_COUCHBASE_SCOPE}" =~ ${couchbase_name_pattern} ]]; then
    echo "HVN_COUCHBASE_SCOPE contains unsupported characters" >&2
    exit 1
fi

common_arguments=(
    --cluster "${couchbase_host}:8091"
    --username "${HVN_COUCHBASE_USERNAME}"
    --password "${HVN_COUCHBASE_PASSWORD}"
)

if ! "${couchbase_cli}" server-list "${common_arguments[@]}" >/dev/null 2>&1; then
    "${couchbase_cli}" cluster-init \
        --cluster "${couchbase_host}:8091" \
        --cluster-username "${HVN_COUCHBASE_USERNAME}" \
        --cluster-password "${HVN_COUCHBASE_PASSWORD}" \
        --services data,index,query \
        --cluster-ramsize 512 \
        --cluster-index-ramsize 256
fi

if ! "${couchbase_cli}" bucket-list "${common_arguments[@]}" |
    awk '{print $1}' |
    grep --fixed-strings --line-regexp --quiet "${HVN_COUCHBASE_BUCKET}"; then
    "${couchbase_cli}" bucket-create \
        "${common_arguments[@]}" \
        --bucket "${HVN_COUCHBASE_BUCKET}" \
        --bucket-type couchbase \
        --bucket-ramsize 256 \
        --bucket-replica 0 \
        --storage-backend couchstore \
        --wait
fi

if ! "${couchbase_cli}" collection-manage \
    "${common_arguments[@]}" \
    --bucket "${HVN_COUCHBASE_BUCKET}" \
    --list-scopes |
    grep --fixed-strings --line-regexp --quiet "${HVN_COUCHBASE_SCOPE}"; then
    "${couchbase_cli}" collection-manage \
        "${common_arguments[@]}" \
        --bucket "${HVN_COUCHBASE_BUCKET}" \
        --create-scope "${HVN_COUCHBASE_SCOPE}"
fi

for collection_name in resources reservations; do
    if ! "${couchbase_cli}" collection-manage \
        "${common_arguments[@]}" \
        --bucket "${HVN_COUCHBASE_BUCKET}" \
        --list-collections "${HVN_COUCHBASE_SCOPE}" |
        grep --fixed-strings --quiet "${collection_name}"; then
        "${couchbase_cli}" collection-manage \
            "${common_arguments[@]}" \
            --bucket "${HVN_COUCHBASE_BUCKET}" \
            --create-collection "${HVN_COUCHBASE_SCOPE}.${collection_name}"
    fi
done

rendered_index_file="$(mktemp)"
readonly rendered_index_file
trap 'rm -f "${rendered_index_file}"' EXIT

sed \
    -e "s/{{BUCKET}}/${HVN_COUCHBASE_BUCKET}/g" \
    -e "s/{{SCOPE}}/${HVN_COUCHBASE_SCOPE}/g" \
    "${index_file}" >"${rendered_index_file}"

"${cbq}" \
    --engine "http://${couchbase_host}:8093" \
    --user "${HVN_COUCHBASE_USERNAME}" \
    --password "${HVN_COUCHBASE_PASSWORD}" \
    --exit-on-error \
    --file "${rendered_index_file}"

echo "Haven Couchbase bucket, scope, collections, and indexes are ready"
