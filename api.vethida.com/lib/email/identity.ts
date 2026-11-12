// Postal address for the email footer (just the address, the templates print the
// company name). Empty if LEGAL_ENTITY_ADDRESS isn't set, the footer drops it.
export function legalEntityAddress(): string {
  return process.env.LEGAL_ENTITY_ADDRESS?.trim() ?? "";
}
