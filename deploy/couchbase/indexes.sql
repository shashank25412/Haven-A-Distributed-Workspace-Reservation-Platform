-- Secondary indexes required by Haven's scope-level Couchbase repository queries.
-- Run in the configured bucket and scope, replacing neither collection names nor fields.

CREATE INDEX `idx_reservations_tenant_resource_interval_status`
ON `reservations`(`organizationId`, `resourceId`, `startTime`, `endTime`, `status`)
WHERE `documentType` = "reservation";

CREATE INDEX `idx_reservations_tenant_creator`
ON `reservations`(`organizationId`, `createdBy`)
WHERE `documentType` = "reservation";

CREATE INDEX `idx_reservations_tenant_status`
ON `reservations`(`organizationId`, `status`)
WHERE `documentType` = "reservation";
