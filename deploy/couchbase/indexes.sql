-- Secondary indexes required by Haven's Couchbase repository queries.
-- The local bootstrap replaces the bucket and scope placeholders with validated
-- environment configuration before executing these idempotent statements.

CREATE INDEX IF NOT EXISTS `idx_resources_tenant_type_status`
ON `{{BUCKET}}`.`{{SCOPE}}`.`resources`(`organizationId`, `resourceType`, `status`)
WHERE `documentType` = "resource";

CREATE INDEX IF NOT EXISTS `idx_reservations_tenant_resource_interval_status`
ON `{{BUCKET}}`.`{{SCOPE}}`.`reservations`(
    `organizationId`, `resourceId`, `status`, `startTime`, `endTime`)
WHERE `documentType` = "reservation";

CREATE INDEX IF NOT EXISTS `idx_reservations_tenant_creator`
ON `{{BUCKET}}`.`{{SCOPE}}`.`reservations`(`organizationId`, `createdBy`)
WHERE `documentType` = "reservation";

CREATE INDEX IF NOT EXISTS `idx_reservations_tenant_status`
ON `{{BUCKET}}`.`{{SCOPE}}`.`reservations`(`organizationId`, `status`)
WHERE `documentType` = "reservation";

CREATE INDEX IF NOT EXISTS `idx_outbox_pending_order`
ON `{{BUCKET}}`.`{{SCOPE}}`.`outbox`(`status`, `occurredAt`, `eventId`)
WHERE `documentType` = "outbox";

CREATE INDEX IF NOT EXISTS `idx_organizations_directory`
ON `{{BUCKET}}`.`{{SCOPE}}`.`organizations`(`name`)
WHERE `documentType` = "organization";

CREATE INDEX IF NOT EXISTS `idx_credentials_user_id`
ON `{{IDENTITY_BUCKET}}`.`{{IDENTITY_SCOPE}}`.`credentials`(`userId`)
WHERE `documentType` = "userCredential";
