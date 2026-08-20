FROM couchbase/server:community-8.0.2

COPY initialize.sh indexes.sql seed-resources.sql seed-resources-secondary.sql seed-organizations.sql /opt/haven/couchbase/

ENTRYPOINT ["/bin/bash", "/opt/haven/couchbase/initialize.sh"]
