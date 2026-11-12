-- Database roles. Run once on a new database.
--   vethida_app  the app (DML only)
--   vethida_ro   read-only, for support queries
-- Migrations run as the owner role.

\set ON_ERROR_STOP on

CREATE ROLE vethida_app LOGIN NOINHERIT PASSWORD :'app_password';

CREATE ROLE vethida_ro LOGIN NOINHERIT PASSWORD :'ro_password';

-- nobody gets to create tables in public by default
REVOKE ALL ON SCHEMA public FROM PUBLIC;
GRANT USAGE ON SCHEMA public TO vethida_app, vethida_ro;

GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO vethida_app;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO vethida_app;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO vethida_ro;

-- tables created by later migrations
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO vethida_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT USAGE, SELECT ON SEQUENCES TO vethida_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA public
  GRANT SELECT ON TABLES TO vethida_ro;

-- no TRUNCATE or DDL for the app role
