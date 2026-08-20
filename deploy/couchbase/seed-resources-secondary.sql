-- Local development catalog for the secondary seed organization.
-- Stable document keys make this safe to run every time couchbase-init starts.

UPSERT INTO `{{BUCKET}}`.`{{SCOPE}}`.`resources`
    (KEY resource_key, VALUE resource_document)
SELECT
    "resource::{{SEED_SECONDARY_ORGANIZATION_ID}}::" || seed.id AS resource_key,
    {
        "documentType": "resource",
        "schemaVersion": 1,
        "resourceId": seed.id,
        "organizationId": "{{SEED_SECONDARY_ORGANIZATION_ID}}",
        "name": seed.name,
        "description": seed.description,
        "resourceType": seed.resourceType,
        "status": seed.status,
        "requiresApproval": seed.requiresApproval,
        "version": 1,
        "totalUnits": seed.totalUnits,
        "capacity": seed.capacity,
        "location": seed.location,
        "features": seed.features,
        "tags": seed.tags
    } AS resource_document
FROM [
    {"id":"meeting-room-birch","name":"Birch Conference Room","description":"Bright conference room for cross-team syncs.","resourceType":"MEETING_ROOM","status":"ACTIVE","requiresApproval":false,"capacity":10,"location":{"building":"Riverside Campus","floor":"3","zone":"East"},"features":["VIDEO_CONFERENCING","WHITEBOARD"],"tags":["TEAM"]},
    {"id":"meeting-room-cascade","name":"Cascade Boardroom","description":"Executive boardroom overlooking the river.","resourceType":"MEETING_ROOM","status":"ACTIVE","requiresApproval":true,"capacity":14,"location":{"building":"Riverside Campus","floor":"5","zone":"West"},"features":["VIDEO_CONFERENCING","DUAL_DISPLAY"],"tags":["EXECUTIVE"]},
    {"id":"office-desk-r01","name":"Riverside Desk R01","description":"Standing desk with dual monitors near the atrium.","resourceType":"OFFICE_DESK","status":"ACTIVE","requiresApproval":false,"capacity":1,"location":{"building":"Riverside Campus","floor":"2","zone":"Atrium"},"features":["STANDING_DESK","DUAL_MONITOR"],"tags":["WINDOW"]},
    {"id":"office-desk-r02","name":"Riverside Desk R02","description":"Quiet desk in the design neighborhood.","resourceType":"OFFICE_DESK","status":"ACTIVE","requiresApproval":false,"capacity":1,"location":{"building":"Riverside Campus","floor":"2","zone":"Design"},"features":["DUAL_MONITOR","DOCK"],"tags":["QUIET"]},
    {"id":"parking-slot-r01","name":"Riverside Parking R01","description":"Covered parking spot near the main entrance.","resourceType":"PARKING_SLOT","status":"ACTIVE","requiresApproval":false,"capacity":1,"location":{"building":"Riverside Campus","floor":"G","zone":"Main Entrance"},"features":["COVERED"],"tags":["ACCESSIBLE"]},
    {"id":"game-zone-r01","name":"Riverside Game Zone","description":"Recreation room with table tennis and a lounge area.","resourceType":"GAME_ZONE","status":"ACTIVE","requiresApproval":false,"capacity":8,"location":{"building":"Riverside Campus","floor":"1","zone":"Recreation"},"features":["TABLE_TENNIS","LOUNGE"],"tags":["RECREATION"]},
    {"id":"parking-pool-riverside","name":"Riverside Parking Pool","description":"Unassigned overflow parking pool shared by every visitor to the Riverside Campus lot.","resourceType":"PARKING_SLOT","status":"ACTIVE","requiresApproval":false,"capacity":1,"totalUnits":12,"location":{"building":"Riverside Campus","floor":"G","zone":"Overflow Lot"},"features":["COVERED"],"tags":["POOL","STANDARD"]}
] AS seed;
