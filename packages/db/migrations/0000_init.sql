CREATE TABLE "account" (
	"id" text PRIMARY KEY NOT NULL,
	"account_id" text NOT NULL,
	"provider_id" text NOT NULL,
	"user_id" text NOT NULL,
	"access_token" text,
	"refresh_token" text,
	"id_token" text,
	"access_token_expires_at" timestamp,
	"refresh_token_expires_at" timestamp,
	"scope" text,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp NOT NULL
);
--> statement-breakpoint
CREATE TABLE "rate_limit" (
	"id" text PRIMARY KEY NOT NULL,
	"key" text NOT NULL,
	"count" integer NOT NULL,
	"last_request" bigint NOT NULL,
	CONSTRAINT "rate_limit_key_unique" UNIQUE("key")
);
--> statement-breakpoint
CREATE TABLE "auth_session" (
	"id" text PRIMARY KEY NOT NULL,
	"expires_at" timestamp NOT NULL,
	"token" text NOT NULL,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp NOT NULL,
	"ip_address" text,
	"user_agent" text,
	"user_id" text NOT NULL,
	CONSTRAINT "auth_session_token_unique" UNIQUE("token")
);
--> statement-breakpoint
CREATE TABLE "user" (
	"id" text PRIMARY KEY NOT NULL,
	"name" text NOT NULL,
	"email" text NOT NULL,
	"email_verified" boolean DEFAULT false NOT NULL,
	"image" text,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp DEFAULT now() NOT NULL,
	"role" text DEFAULT 'user' NOT NULL,
	"completed_onboarding" boolean DEFAULT false NOT NULL,
	CONSTRAINT "user_email_unique" UNIQUE("email")
);
--> statement-breakpoint
CREATE TABLE "verification" (
	"id" text PRIMARY KEY NOT NULL,
	"identifier" text NOT NULL,
	"value" text NOT NULL,
	"expires_at" timestamp NOT NULL,
	"created_at" timestamp DEFAULT now() NOT NULL,
	"updated_at" timestamp DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "desktop_auth_session" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"user_code" varchar(9) NOT NULL,
	"device_code_hash" text NOT NULL,
	"device_public_key" "bytea" NOT NULL,
	"device_pubkey_id" varchar(32) NOT NULL,
	"hardware_fingerprint" jsonb NOT NULL,
	"status" varchar(16) DEFAULT 'pending' NOT NULL,
	"user_id" text,
	"license_id" uuid,
	"start_ip" varchar(45),
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"expires_at" timestamp with time zone NOT NULL,
	"approved_at" timestamp with time zone,
	"consumed_at" timestamp with time zone,
	CONSTRAINT "desktop_auth_status_check" CHECK (status IN ('pending', 'approved', 'denied', 'consumed')),
	CONSTRAINT "desktop_auth_approved_consistency_check" CHECK (status <> 'approved' OR (user_id IS NOT NULL AND license_id IS NOT NULL)),
	CONSTRAINT "desktop_auth_public_key_length_check" CHECK (octet_length(device_public_key) = 64)
);
--> statement-breakpoint
CREATE TABLE "device" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"license_id" uuid NOT NULL,
	"user_id" text,
	"device_public_key" "bytea" NOT NULL,
	"device_pubkey_id" varchar(32) NOT NULL,
	"hardware_fingerprint" jsonb NOT NULL,
	"last_ip" varchar(45),
	"deactivated_at" timestamp with time zone,
	"last_seen_at" timestamp with time zone DEFAULT now() NOT NULL,
	"activated_at" timestamp with time zone DEFAULT now() NOT NULL,
	"fingerprint_notified_at" timestamp with time zone,
	CONSTRAINT "device_public_key_length_check" CHECK (octet_length(device_public_key) = 64)
);
--> statement-breakpoint
CREATE TABLE "license" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"key" text NOT NULL,
	"key_hash" text NOT NULL,
	"tier" varchar(20) NOT NULL,
	"trial" boolean DEFAULT false NOT NULL,
	"user_id" text,
	"machine_resets_remaining" integer DEFAULT 3 NOT NULL,
	"token_generation" integer DEFAULT 0 NOT NULL,
	"polar_order_id" text,
	"polar_subscription_id" text,
	"cancel_at_period_end" boolean DEFAULT false NOT NULL,
	"superseded_at" timestamp with time zone,
	"billing_interval" varchar(10),
	"expires_at" timestamp with time zone,
	"revoked_at" timestamp with time zone,
	"revoked_by" text,
	"revoked_reason_code" varchar(32),
	"revoked_reason" text,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"renewal_reminder_sent_at" timestamp with time zone,
	"past_due_notice_sent_at" timestamp with time zone,
	CONSTRAINT "license_tier_check" CHECK (tier IN ('pro', 'ultimate')),
	CONSTRAINT "license_trial_expiry_check" CHECK (trial = false OR expires_at IS NOT NULL),
	CONSTRAINT "license_trial_unpaid_check" CHECK (trial = false OR (polar_order_id IS NULL AND polar_subscription_id IS NULL)),
	CONSTRAINT "license_machine_resets_check" CHECK (machine_resets_remaining >= 0),
	CONSTRAINT "license_token_generation_check" CHECK (token_generation >= 0),
	CONSTRAINT "license_revoked_reason_code_check" CHECK (revoked_reason_code IS NULL OR revoked_reason_code IN (
        'chargeback', 'tos_violation', 'fraud', 'user_request', 'admin_error', 'other'
      )),
	CONSTRAINT "license_revoked_consistency_check" CHECK ((revoked_at IS NULL AND revoked_reason_code IS NULL)
          OR (revoked_at IS NOT NULL AND revoked_reason_code IS NOT NULL))
);
--> statement-breakpoint
CREATE TABLE "user_suspension" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"user_id" text NOT NULL,
	"reason_code" varchar(32) NOT NULL,
	"reason_detail" text,
	"source" varchar(32) NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"lifted_at" timestamp with time zone,
	"lifted_by" text,
	CONSTRAINT "user_suspension_reason_check" CHECK (reason_code IN ('device_sharing', 'chargeback', 'admin_action')),
	CONSTRAINT "user_suspension_source_check" CHECK (source IN ('auto_antishare', 'chargeback_webhook', 'admin'))
);
--> statement-breakpoint
CREATE TABLE "webhook_event" (
	"provider" varchar(32) NOT NULL,
	"event_id" text NOT NULL,
	"event_type" text NOT NULL,
	"status" varchar(16) DEFAULT 'received' NOT NULL,
	"received_at" timestamp with time zone DEFAULT now() NOT NULL,
	"processed_at" timestamp with time zone,
	"expires_at" timestamp with time zone NOT NULL,
	CONSTRAINT "webhook_event_provider_event_id_pk" PRIMARY KEY("provider","event_id"),
	CONSTRAINT "webhook_event_status_check" CHECK (status IN ('received', 'processed'))
);
--> statement-breakpoint
CREATE TABLE "abuse_flag" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"user_id" text,
	"license_id" uuid,
	"device_id" uuid,
	"type" varchar(40) NOT NULL,
	"details" jsonb,
	"resolved_at" timestamp with time zone,
	"resolved_by" text,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "abuse_flag_type_check" CHECK (type IN (
        'fingerprint_flag',
        'supersede_failed',
        'supersede_not_honoured',
        'superseded_renewal_charged'
      ))
);
--> statement-breakpoint
CREATE TABLE "admin_audit_log" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"admin_id" text,
	"actor" varchar(16) DEFAULT 'admin' NOT NULL,
	"action" varchar(64) NOT NULL,
	"target_type" varchar(32) NOT NULL,
	"target_id" text NOT NULL,
	"ip_address" varchar(45),
	"details" jsonb,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "admin_audit_log_actor_check" CHECK (actor IN ('admin', 'system')),
	CONSTRAINT "admin_audit_log_admin_ip_check" CHECK (actor <> 'admin' OR ip_address IS NOT NULL),
	CONSTRAINT "admin_audit_log_system_actor_check" CHECK (actor <> 'system' OR admin_id IS NULL)
);
--> statement-breakpoint
CREATE TABLE "account_deletion" (
	"user_id" text PRIMARY KEY NOT NULL,
	"requested_at" timestamp with time zone DEFAULT now() NOT NULL,
	"hard_delete_at" timestamp with time zone NOT NULL,
	"processing_restricted" boolean DEFAULT true NOT NULL,
	"blocked_reason" varchar(32),
	"cancelled_subscription_ids" text[]
);
--> statement-breakpoint
CREATE TABLE "deletion_audit" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"event" varchar(24) NOT NULL,
	"user_ref" text NOT NULL,
	"at" timestamp with time zone DEFAULT now() NOT NULL,
	"detail" text
);
--> statement-breakpoint
CREATE TABLE "email_preference" (
	"user_id" text PRIMARY KEY NOT NULL,
	"fingerprint_alerts" boolean DEFAULT true NOT NULL,
	"release_announcements" boolean DEFAULT false NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "polar_customer" (
	"user_id" text PRIMARY KEY NOT NULL,
	"polar_customer_id" text NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "polar_customer_polar_customer_id_unique" UNIQUE("polar_customer_id")
);
--> statement-breakpoint
CREATE TABLE "cancellation_request" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"user_id" text,
	"license_id" uuid,
	"polar_subscription_id" text,
	"tier" varchar(20) NOT NULL,
	"reason" varchar(32) NOT NULL,
	"comment" text,
	"feedback_at" timestamp with time zone,
	"offer_shown" varchar(32),
	"outcome" varchar(32) DEFAULT 'pending' NOT NULL,
	"offer_detail" jsonb,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"resolved_at" timestamp with time zone,
	CONSTRAINT "cancellation_request_reason_check" CHECK (reason IN ('too_expensive', 'missing_features', 'switched_service', 'unused', 'customer_service', 'low_quality', 'too_complex', 'other')),
	CONSTRAINT "cancellation_request_outcome_check" CHECK (outcome IN ('pending', 'saved_discount', 'saved_downgrade', 'saved_support', 'declined_offer', 'canceled', 'abandoned'))
);
--> statement-breakpoint
CREATE TABLE "discord_ai_usage" (
	"user_id" varchar(32) NOT NULL,
	"day" varchar(10) NOT NULL,
	"input_tokens" bigint DEFAULT 0 NOT NULL,
	"output_tokens" bigint DEFAULT 0 NOT NULL,
	"requests" integer DEFAULT 0 NOT NULL,
	"last_used_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "discord_ai_usage_user_id_day_pk" PRIMARY KEY("user_id","day")
);
--> statement-breakpoint
CREATE TABLE "discord_automod_event" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"guild_id" varchar(32) NOT NULL,
	"user_id" varchar(32) NOT NULL,
	"kind" varchar(32) NOT NULL,
	"content" text,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "discord_mod_action" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"guild_id" varchar(32) NOT NULL,
	"user_id" varchar(32) NOT NULL,
	"action" varchar(32) NOT NULL,
	"moderator" varchar(32) NOT NULL,
	"reason" text NOT NULL,
	"metadata" jsonb,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE TABLE "discord_ticket" (
	"id" text PRIMARY KEY NOT NULL,
	"guild_id" varchar(32) NOT NULL,
	"user_id" varchar(32) NOT NULL,
	"category" varchar(32) NOT NULL,
	"thread_id" varchar(32) NOT NULL,
	"status" varchar(16) DEFAULT 'open' NOT NULL,
	"opened_at" timestamp with time zone DEFAULT now() NOT NULL,
	"closed_at" timestamp with time zone,
	"closed_by" varchar(32),
	"resolution" text
);
--> statement-breakpoint
CREATE TABLE "discord_warning" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"guild_id" varchar(32) NOT NULL,
	"user_id" varchar(32) NOT NULL,
	"moderator" varchar(32) NOT NULL,
	"reason" text NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
ALTER TABLE "account" ADD CONSTRAINT "account_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "auth_session" ADD CONSTRAINT "auth_session_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "desktop_auth_session" ADD CONSTRAINT "desktop_auth_session_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "desktop_auth_session" ADD CONSTRAINT "desktop_auth_session_license_id_license_id_fk" FOREIGN KEY ("license_id") REFERENCES "public"."license"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "device" ADD CONSTRAINT "device_license_id_license_id_fk" FOREIGN KEY ("license_id") REFERENCES "public"."license"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "device" ADD CONSTRAINT "device_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "license" ADD CONSTRAINT "license_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "license" ADD CONSTRAINT "license_revoked_by_user_id_fk" FOREIGN KEY ("revoked_by") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "user_suspension" ADD CONSTRAINT "user_suspension_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "user_suspension" ADD CONSTRAINT "user_suspension_lifted_by_user_id_fk" FOREIGN KEY ("lifted_by") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "abuse_flag" ADD CONSTRAINT "abuse_flag_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "abuse_flag" ADD CONSTRAINT "abuse_flag_license_id_license_id_fk" FOREIGN KEY ("license_id") REFERENCES "public"."license"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "abuse_flag" ADD CONSTRAINT "abuse_flag_device_id_device_id_fk" FOREIGN KEY ("device_id") REFERENCES "public"."device"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "abuse_flag" ADD CONSTRAINT "abuse_flag_resolved_by_user_id_fk" FOREIGN KEY ("resolved_by") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "admin_audit_log" ADD CONSTRAINT "admin_audit_log_admin_id_user_id_fk" FOREIGN KEY ("admin_id") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "account_deletion" ADD CONSTRAINT "account_deletion_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "email_preference" ADD CONSTRAINT "email_preference_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "polar_customer" ADD CONSTRAINT "polar_customer_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "cancellation_request" ADD CONSTRAINT "cancellation_request_user_id_user_id_fk" FOREIGN KEY ("user_id") REFERENCES "public"."user"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "cancellation_request" ADD CONSTRAINT "cancellation_request_license_id_license_id_fk" FOREIGN KEY ("license_id") REFERENCES "public"."license"("id") ON DELETE set null ON UPDATE no action;--> statement-breakpoint
CREATE INDEX "account_userId_idx" ON "account" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "auth_session_user_idx" ON "auth_session" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "auth_session_expires_idx" ON "auth_session" USING btree ("expires_at");--> statement-breakpoint
CREATE INDEX "verification_identifier_idx" ON "verification" USING btree ("identifier");--> statement-breakpoint
CREATE UNIQUE INDEX "desktop_auth_device_code_unique" ON "desktop_auth_session" USING btree ("device_code_hash");--> statement-breakpoint
CREATE UNIQUE INDEX "desktop_auth_user_code_live_unique" ON "desktop_auth_session" USING btree ("user_code") WHERE status = 'pending';--> statement-breakpoint
CREATE INDEX "desktop_auth_expires_idx" ON "desktop_auth_session" USING btree ("expires_at");--> statement-breakpoint
CREATE INDEX "desktop_auth_user_id_idx" ON "desktop_auth_session" USING btree ("user_id");--> statement-breakpoint
CREATE UNIQUE INDEX "device_pubkey_id_unique" ON "device" USING btree ("device_pubkey_id");--> statement-breakpoint
CREATE INDEX "device_user_id_idx" ON "device" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "device_license_active_idx" ON "device" USING btree ("license_id") WHERE deactivated_at IS NULL;--> statement-breakpoint
CREATE INDEX "device_license_id_idx" ON "device" USING btree ("license_id");--> statement-breakpoint
CREATE UNIQUE INDEX "license_key_unique" ON "license" USING btree ("key");--> statement-breakpoint
CREATE UNIQUE INDEX "license_key_hash_unique" ON "license" USING btree ("key_hash");--> statement-breakpoint
CREATE UNIQUE INDEX "license_polar_order_unique" ON "license" USING btree ("polar_order_id") WHERE polar_order_id IS NOT NULL;--> statement-breakpoint
CREATE UNIQUE INDEX "license_polar_subscription_unique" ON "license" USING btree ("polar_subscription_id") WHERE polar_subscription_id IS NOT NULL;--> statement-breakpoint
CREATE INDEX "license_user_id_idx" ON "license" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "license_created_at_idx" ON "license" USING btree ("created_at");--> statement-breakpoint
CREATE UNIQUE INDEX "license_one_trial_per_user" ON "license" USING btree ("user_id") WHERE trial = true AND user_id IS NOT NULL;--> statement-breakpoint
CREATE UNIQUE INDEX "user_suspension_active_unique" ON "user_suspension" USING btree ("user_id","reason_code") WHERE lifted_at IS NULL;--> statement-breakpoint
CREATE INDEX "user_suspension_user_history_idx" ON "user_suspension" USING btree ("user_id","created_at");--> statement-breakpoint
CREATE INDEX "webhook_event_expires_idx" ON "webhook_event" USING btree ("expires_at");--> statement-breakpoint
CREATE INDEX "abuse_flag_unresolved_idx" ON "abuse_flag" USING btree ("created_at") WHERE resolved_at IS NULL;--> statement-breakpoint
CREATE INDEX "abuse_flag_user_idx" ON "abuse_flag" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "abuse_flag_license_idx" ON "abuse_flag" USING btree ("license_id");--> statement-breakpoint
CREATE INDEX "admin_audit_log_admin_idx" ON "admin_audit_log" USING btree ("admin_id","created_at");--> statement-breakpoint
CREATE INDEX "admin_audit_log_target_idx" ON "admin_audit_log" USING btree ("target_type","target_id","created_at");--> statement-breakpoint
CREATE INDEX "admin_audit_log_created_idx" ON "admin_audit_log" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "account_deletion_hard_delete_idx" ON "account_deletion" USING btree ("hard_delete_at");--> statement-breakpoint
CREATE INDEX "cancellation_request_user_idx" ON "cancellation_request" USING btree ("user_id");--> statement-breakpoint
CREATE INDEX "cancellation_request_created_idx" ON "cancellation_request" USING btree ("created_at");--> statement-breakpoint
CREATE INDEX "cancellation_request_outcome_idx" ON "cancellation_request" USING btree ("outcome");--> statement-breakpoint
CREATE INDEX "discord_automod_event_user_idx" ON "discord_automod_event" USING btree ("guild_id","user_id","created_at");--> statement-breakpoint
CREATE INDEX "discord_mod_action_guild_user_idx" ON "discord_mod_action" USING btree ("guild_id","user_id","created_at");--> statement-breakpoint
CREATE INDEX "discord_ticket_open_idx" ON "discord_ticket" USING btree ("guild_id","user_id","status");--> statement-breakpoint
CREATE INDEX "discord_warning_guild_user_idx" ON "discord_warning" USING btree ("guild_id","user_id","created_at");