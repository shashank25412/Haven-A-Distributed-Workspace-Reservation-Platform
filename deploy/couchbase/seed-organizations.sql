-- Local development organization directory.
-- Stable document keys make this safe to run every time couchbase-init starts.

UPSERT INTO `{{BUCKET}}`.`{{SCOPE}}`.`organizations`
    (KEY organization_key, VALUE organization_document)
SELECT
    "organization::" || seed.id AS organization_key,
    {
        "documentType": "organization",
        "schemaVersion": 1,
        "organizationId": seed.id,
        "name": seed.name,
        "imageUrl": seed.imageUrl,
        "rank": seed.rank
    } AS organization_document
FROM [
    {"id":"{{SEED_ORGANIZATION_ID}}","name":"{{SEED_ORGANIZATION_NAME}}","imageUrl":"/images/organization-defaults/organization-one.jpg","rank":1},
    {"id":"{{SEED_SECONDARY_ORGANIZATION_ID}}","name":"{{SEED_SECONDARY_ORGANIZATION_NAME}}","imageUrl":"/images/organization-defaults/organization-two.jpg","rank":2}
] AS seed;
