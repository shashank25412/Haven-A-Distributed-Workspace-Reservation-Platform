#!/usr/bin/env bash

set -euo pipefail

readonly couchbase_host="${HVN_COUCHBASE_HOST:-couchbase}"
readonly couchbase_cli="/opt/couchbase/bin/couchbase-cli"
readonly cbq="/opt/couchbase/bin/cbq"
readonly index_file="${HVN_COUCHBASE_INDEX_FILE:-/opt/haven/couchbase/indexes.sql}"
readonly resource_seed_file="${HVN_COUCHBASE_RESOURCE_SEED_FILE:-/opt/haven/couchbase/seed-resources.sql}"
readonly secondary_resource_seed_file="${HVN_COUCHBASE_SECONDARY_RESOURCE_SEED_FILE:-/opt/haven/couchbase/seed-resources-secondary.sql}"
readonly organization_seed_file="${HVN_COUCHBASE_ORGANIZATION_SEED_FILE:-/opt/haven/couchbase/seed-organizations.sql}"
readonly seed_organization_id="${HVN_SEED_ORGANIZATION_ID:-organization-1}"
readonly seed_organization_name="${HVN_SEED_ORGANIZATION_NAME:-Organization One}"
readonly seed_secondary_organization_id="${HVN_SEED_SECONDARY_ORGANIZATION_ID:-organization-2}"
readonly seed_secondary_organization_name="${HVN_SEED_SECONDARY_ORGANIZATION_NAME:-Organization Two}"
readonly identity_bucket="${HVN_IDENTITY_COUCHBASE_BUCKET:-${HVN_COUCHBASE_BUCKET}_identity}"
readonly identity_scope="${HVN_IDENTITY_COUCHBASE_SCOPE:-identity}"

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
if [[ ! "${identity_bucket}" =~ ${couchbase_name_pattern} ]]; then
    echo "HVN_IDENTITY_COUCHBASE_BUCKET contains unsupported characters" >&2
    exit 1
fi
if [[ ! "${identity_scope}" =~ ${couchbase_name_pattern} ]]; then
    echo "HVN_IDENTITY_COUCHBASE_SCOPE contains unsupported characters" >&2
    exit 1
fi

readonly seed_organization_id_pattern='^[A-Za-z0-9_-]+$'
if [[ ! "${seed_organization_id}" =~ ${seed_organization_id_pattern} ]]; then
    echo "HVN_SEED_ORGANIZATION_ID contains unsupported characters" >&2
    exit 1
fi
if [[ ! "${seed_secondary_organization_id}" =~ ${seed_organization_id_pattern} ]]; then
    echo "HVN_SEED_SECONDARY_ORGANIZATION_ID contains unsupported characters" >&2
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

if ! "${couchbase_cli}" bucket-list "${common_arguments[@]}" |
    awk '{print $1}' |
    grep --fixed-strings --line-regexp --quiet "${identity_bucket}"; then
    "${couchbase_cli}" bucket-create \
        "${common_arguments[@]}" \
        --bucket "${identity_bucket}" \
        --bucket-type couchbase \
        --bucket-ramsize 128 \
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

for collection_name in resources reservations idempotency outbox organizations; do
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

if ! "${couchbase_cli}" collection-manage \
    "${common_arguments[@]}" \
    --bucket "${identity_bucket}" \
    --list-scopes |
    grep --fixed-strings --line-regexp --quiet "${identity_scope}"; then
    "${couchbase_cli}" collection-manage \
        "${common_arguments[@]}" \
        --bucket "${identity_bucket}" \
        --create-scope "${identity_scope}"
fi

for collection_name in credentials sessions; do
    if ! "${couchbase_cli}" collection-manage \
        "${common_arguments[@]}" \
        --bucket "${identity_bucket}" \
        --list-collections "${identity_scope}" |
        grep --fixed-strings --quiet "${collection_name}"; then
        "${couchbase_cli}" collection-manage \
            "${common_arguments[@]}" \
            --bucket "${identity_bucket}" \
            --create-collection "${identity_scope}.${collection_name}"
    fi
done

rendered_index_file="$(mktemp)"
readonly rendered_index_file
trap 'rm -f "${rendered_index_file}"' EXIT

sed \
    -e "s/{{BUCKET}}/${HVN_COUCHBASE_BUCKET}/g" \
    -e "s/{{SCOPE}}/${HVN_COUCHBASE_SCOPE}/g" \
    -e "s/{{IDENTITY_BUCKET}}/${identity_bucket}/g" \
    -e "s/{{IDENTITY_SCOPE}}/${identity_scope}/g" \
    "${index_file}" >"${rendered_index_file}"

"${cbq}" \
    --engine "http://${couchbase_host}:8093" \
    --user "${HVN_COUCHBASE_USERNAME}" \
    --password "${HVN_COUCHBASE_PASSWORD}" \
    --exit-on-error \
    --file "${rendered_index_file}"

rendered_seed_file="$(mktemp)"
readonly rendered_seed_file
trap 'rm -f "${rendered_index_file}" "${rendered_seed_file}"' EXIT

sed \
    -e "s/{{BUCKET}}/${HVN_COUCHBASE_BUCKET}/g" \
    -e "s/{{SCOPE}}/${HVN_COUCHBASE_SCOPE}/g" \
    -e "s/{{SEED_ORGANIZATION_ID}}/${seed_organization_id}/g" \
    "${resource_seed_file}" >"${rendered_seed_file}"

"${cbq}" \
    --engine "http://${couchbase_host}:8093" \
    --user "${HVN_COUCHBASE_USERNAME}" \
    --password "${HVN_COUCHBASE_PASSWORD}" \
    --exit-on-error \
    --file "${rendered_seed_file}"

rendered_secondary_seed_file="$(mktemp)"
readonly rendered_secondary_seed_file
trap 'rm -f "${rendered_index_file}" "${rendered_seed_file}" "${rendered_secondary_seed_file}"' EXIT

sed \
    -e "s/{{BUCKET}}/${HVN_COUCHBASE_BUCKET}/g" \
    -e "s/{{SCOPE}}/${HVN_COUCHBASE_SCOPE}/g" \
    -e "s/{{SEED_SECONDARY_ORGANIZATION_ID}}/${seed_secondary_organization_id}/g" \
    "${secondary_resource_seed_file}" >"${rendered_secondary_seed_file}"

"${cbq}" \
    --engine "http://${couchbase_host}:8093" \
    --user "${HVN_COUCHBASE_USERNAME}" \
    --password "${HVN_COUCHBASE_PASSWORD}" \
    --exit-on-error \
    --file "${rendered_secondary_seed_file}"

rendered_organization_seed_file="$(mktemp)"
readonly rendered_organization_seed_file
trap 'rm -f "${rendered_index_file}" "${rendered_seed_file}" "${rendered_secondary_seed_file}" "${rendered_organization_seed_file}"' EXIT

sed \
    -e "s/{{BUCKET}}/${HVN_COUCHBASE_BUCKET}/g" \
    -e "s/{{SCOPE}}/${HVN_COUCHBASE_SCOPE}/g" \
    -e "s/{{SEED_ORGANIZATION_ID}}/${seed_organization_id}/g" \
    -e "s/{{SEED_ORGANIZATION_NAME}}/${seed_organization_name}/g" \
    -e "s/{{SEED_SECONDARY_ORGANIZATION_ID}}/${seed_secondary_organization_id}/g" \
    -e "s/{{SEED_SECONDARY_ORGANIZATION_NAME}}/${seed_secondary_organization_name}/g" \
    "${organization_seed_file}" >"${rendered_organization_seed_file}"

"${cbq}" \
    --engine "http://${couchbase_host}:8093" \
    --user "${HVN_COUCHBASE_USERNAME}" \
    --password "${HVN_COUCHBASE_PASSWORD}" \
    --exit-on-error \
    --file "${rendered_organization_seed_file}"

echo "Haven reservation and identity buckets, collections, indexes, and local resource seed data are ready"
