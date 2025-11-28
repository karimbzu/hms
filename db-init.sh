#!/usr/bin/env bash
set -euo pipefail

DB_DIR="/app/data"
DB_PATH="$DB_DIR/hospital.db"

mkdir -p "$DB_DIR"

if [ ! -f "$DB_PATH" ]; then
  echo "Initializing database at $DB_PATH"
  sqlite3 "$DB_PATH" <<'SQL'
CREATE TABLE IF NOT EXISTS doctors (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  specialty TEXT
);
CREATE TABLE IF NOT EXISTS patients (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name TEXT NOT NULL,
  ailment TEXT,
  doctor_id INTEGER DEFAULT 0
);
SQL
else
  echo "Database already exists at $DB_PATH"
fi
