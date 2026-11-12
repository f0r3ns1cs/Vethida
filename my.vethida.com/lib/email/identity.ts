// Postal address for the email footer (same as api.vethida.com). Empty if
// LEGAL_ENTITY_ADDRESS isn't set, the footer drops it.
export function legalEntityAddress(): string {
  return process.env.LEGAL_ENTITY_ADDRESS?.trim() ?? "";
}
