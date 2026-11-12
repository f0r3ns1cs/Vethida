const windows = new Map();

export function allow(key, limit, windowMs) {
  const t = Date.now();
  const w = windows.get(key);
  if (!w || t >= w.resetAt) {
    windows.set(key, { count: 1, resetAt: t + windowMs });
    return true;
  }
  if (w.count >= limit) return false;
  w.count += 1;
  return true;
}

setInterval(() => {
  const t = Date.now();
  for (const [key, w] of windows) if (t >= w.resetAt) windows.delete(key);
}, 5 * 60 * 1000).unref();
