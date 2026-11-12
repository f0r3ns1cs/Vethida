// Dates are always UTC and en-US so server and client render the same string.

const LONG: Intl.DateTimeFormatOptions = {
  year: "numeric",
  month: "long",
  day: "numeric",
  timeZone: "UTC",
};

const SHORT: Intl.DateTimeFormatOptions = {
  year: "numeric",
  month: "short",
  day: "numeric",
  timeZone: "UTC",
};

function asDate(value: Date | string | number): Date {
  return value instanceof Date ? value : new Date(value);
}

// "December 25, 2026"
export function formatDate(value: Date | string | number): string {
  return asDate(value).toLocaleDateString("en-US", LONG);
}

// "Dec 25, 2026"
export function formatDateShort(value: Date | string | number): string {
  return asDate(value).toLocaleDateString("en-US", SHORT);
}
