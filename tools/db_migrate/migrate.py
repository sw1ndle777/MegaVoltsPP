#!/usr/bin/env python3
"""
Bidirectional database migration tool for MegaVoltsPP.
Migrates data between MariaDB and PostgreSQL.

Requirements:
    pip install mysql-connector-python psycopg2-binary

Usage:
    python migrate.py --from mariadb --to postgresql \
        --src-host 127.0.0.1 --src-port 3306 --src-db megavoltspp --src-user root --src-pass god123 \
        --dst-host 127.0.0.1 --dst-port 5433 --dst-db megavoltspp --dst-user megavolts --dst-pass megavolts
"""
import argparse
import sys
import time

try:
    import mysql.connector
except ImportError:
    sys.exit("Missing dependency: pip install mysql-connector-python")

try:
    import psycopg2
    import psycopg2.extras
except ImportError:
    sys.exit("Missing dependency: pip install psycopg2-binary")


BATCH_SIZE = 1000

# (maria_col, pg_col, invert_bool)
# Tables: (maria_table, pg_table, col_renames, skip_maria_cols, has_identity)
TABLE_MAPPINGS = [
    ("accounts", "accounts", [], [], True),
    ("game_titles", "game_titles", [], [], True),
    ("account_titles", "account_titles", [], [], False),
    ("clans", "clans", [], [], True),
    ("clan_member_roles", "clan_member_roles", [], [], False),
    ("bans", "bans", [], [], True),
    ("login_history", "login_history", [], [], True),
    ("player_sessions", "player_sessions", [], [], False),
    ("player_items", "inventory_items", [
        ("PlayerId", "AccountId", False),
        ("SerialInfo", "SerialData", False),
        ("ExpirationDate", "ExpireDate", False),
    ], ["SealLevel", "EnhanceExp", "EnhanceLevel"], False),
    ("player_socials", "socials", [
        ("Aid", "AccountId", False),
        ("TargetAid", "TargetAccountId", False),
        ("State", "Type", False),
    ], [], False),
    ("player_daily_mission", "daily_missions", [
        ("PlayerId", "AccountId", False),
        ("UpdateTime", "LastResetTime", False),
        ("Mission1", "MissionId1", False),
        ("Mission2", "MissionId2", False),
        ("Mission3", "MissionId3", False),
        ("GoalMission1", "Progress1", False),
        ("GoalMission2", "Progress2", False),
        ("GoalMission3", "Progress3", False),
    ], [], False),
    ("player_gacha_pity", "gacha_pity", [
        ("PlayerId", "AccountId", False),
        ("GachaponId", "GachaId", False),
    ], [], False),
    ("player_mailbox", "mailbox", [
        ("ReceiverId", "AccountId", False),
        ("SenderId", "SenderAccountId", False),
        ("GiftItemId", "ItemId", False),
        ("Date", "SentDate", False),
        ("IsNew", "IsRead", True),
    ], ["ReceiverNickname", "DeletedFromSender", "DeletedFromReceiver"], True),
    ("player_monthly_rewards", "player_monthly_rewards", [
        ("PlayerId", "AccountId", False),
        ("RewardCount", "DayCount", False),
        ("LastUpdate", "LastClaimDate", False),
    ], ["ID"], False),
    ("system_monthly_rewards", "system_monthly_rewards", [], [], False),
    ("player_weekly_rewards", "player_weekly_rewards", [
        ("PlayerId", "AccountId", False),
        ("RewardCount", "DayCount", False),
        ("LastUpdate", "LastClaimDate", False),
    ], ["ID"], False),
    ("system_weekly_rewards", "system_weekly_rewards", [], [], False),
    ("system_gachapon_machine", "gachapon_sales", [
        ("GachaponMachineId", "GachaponId", False),
    ], [], False),
    ("player_matchhistory", "player_matchhistory", [
        ("Upper", "Upper_", False),
        ("Under", "Under_", False),
    ], [], True),
    ("player_chatlogs", "log_chat", [], [], True),
    ("player_itemlogs", "log_items", [], [], True),
    ("player_currencylogs", "log_currency", [], [], True),
    ("player_roomlogs", "log_rooms", [], [], True),
    ("ac_detections", "log_ac_detection", [], [], True),
    ("ac_auth_history", "log_auth_history", [], [], True),
]


def rename_col(maria_col: str, renames: list) -> tuple:
    """Returns (pg_col_name, invert_bool)."""
    for mcol, pcol, invert in renames:
        if mcol == maria_col:
            return pcol, invert
    return maria_col, False


def invert_value(val):
    if val is None:
        return None
    if isinstance(val, (bool, int)):
        return not bool(val)
    s = str(val).lower()
    return s in ("0", "false", "", "b'\\x00'")


def migrate_maria_to_pg(src: dict, dst: dict):
    maria = mysql.connector.connect(
        host=src["host"], port=src["port"], database=src["db"],
        user=src["user"], password=src["pass"],
        charset="utf8mb4",
    )
    pg = psycopg2.connect(
        host=dst["host"], port=dst["port"], dbname=dst["db"],
        user=dst["user"], password=dst["pass"],
    )
    pg.autocommit = False
    print(f"Connected to MariaDB ({src['host']}:{src['port']}) and PostgreSQL ({dst['host']}:{dst['port']})")

    total = 0
    t0 = time.time()

    for maria_table, pg_table, col_renames, skip_cols, has_identity in TABLE_MAPPINGS:
        try:
            mc = maria.cursor(dictionary=True)
            mc.execute(f"SELECT * FROM `{maria_table}`")
            rows = mc.fetchall()
            mc.close()

            if not rows:
                print(f"  {maria_table} -> {pg_table} : 0 rows (skipped)")
                continue

            maria_cols = list(rows[0].keys())
            keep = [(c, *rename_col(c, col_renames)) for c in maria_cols if c not in skip_cols]
            pg_cols = [pg_col for _, pg_col, _ in keep]
            invert_flags = [inv for _, _, inv in keep]
            maria_keep = [mc for mc, _, _ in keep]

            col_list = ", ".join(f'"{c}"' for c in pg_cols)
            placeholders = ", ".join(["%s"] * len(pg_cols))
            insert_sql = f'INSERT INTO {pg_table} ({col_list}) VALUES ({placeholders}) ON CONFLICT DO NOTHING'

            if has_identity:
                with pg.cursor() as pc:
                    try:
                        pc.execute(f'ALTER TABLE {pg_table} ALTER COLUMN "Id" DROP IDENTITY IF EXISTS')
                        pg.commit()
                    except Exception:
                        pg.rollback()

            count = 0
            for batch_start in range(0, len(rows), BATCH_SIZE):
                batch = rows[batch_start:batch_start + BATCH_SIZE]
                values = []
                for row in batch:
                    vals = []
                    for i, mcol in enumerate(maria_keep):
                        v = row[mcol]
                        if invert_flags[i]:
                            v = invert_value(v)
                        if isinstance(v, bytes):
                            v = int.from_bytes(v, "big")
                        vals.append(v)
                    values.append(tuple(vals))

                with pg.cursor() as pc:
                    psycopg2.extras.execute_batch(pc, insert_sql, values, page_size=BATCH_SIZE)
                pg.commit()
                count += len(batch)

            if has_identity:
                with pg.cursor() as pc:
                    try:
                        pc.execute(f'SELECT COALESCE(MAX("Id"), 0) + 1 FROM {pg_table}')
                        next_val = pc.fetchone()[0]
                        pc.execute(
                            f'ALTER TABLE {pg_table} ALTER COLUMN "Id" ADD GENERATED ALWAYS AS IDENTITY '
                            f'(START WITH {next_val})'
                        )
                        pg.commit()
                    except Exception:
                        pg.rollback()

            total += count
            print(f"  {maria_table} -> {pg_table} : {count} rows")

        except Exception as e:
            pg.rollback()
            print(f"  {maria_table} -> {pg_table} : FAILED ({e})")

    elapsed = time.time() - t0
    print(f"\nMigration complete: {total} total rows in {elapsed:.1f}s")
    maria.close()
    pg.close()


def migrate_pg_to_maria(src: dict, dst: dict):
    pg = psycopg2.connect(
        host=src["host"], port=src["port"], dbname=src["db"],
        user=src["user"], password=src["pass"],
    )
    maria = mysql.connector.connect(
        host=dst["host"], port=dst["port"], database=dst["db"],
        user=dst["user"], password=dst["pass"],
        charset="utf8mb4",
    )
    maria.autocommit = False
    print(f"Connected to PostgreSQL ({src['host']}:{src['port']}) and MariaDB ({dst['host']}:{dst['port']})")

    total = 0
    t0 = time.time()

    for maria_table, pg_table, col_renames, _skip_cols, _has_identity in TABLE_MAPPINGS:
        try:
            with pg.cursor() as pc:
                pc.execute(f"SELECT * FROM {pg_table}")
                pg_col_names = [desc[0] for desc in pc.description]
                rows = pc.fetchall()

            if not rows:
                print(f"  {pg_table} -> {maria_table} : 0 rows (skipped)")
                continue

            reverse_map = {pcol: (mcol, inv) for mcol, pcol, inv in col_renames}
            maria_cols = []
            invert_flags = []
            for pcol in pg_col_names:
                if pcol in reverse_map:
                    maria_cols.append(reverse_map[pcol][0])
                    invert_flags.append(reverse_map[pcol][1])
                else:
                    maria_cols.append(pcol)
                    invert_flags.append(False)

            col_list = ", ".join(f"`{c}`" for c in maria_cols)
            placeholders = ", ".join(["%s"] * len(maria_cols))
            insert_sql = f"INSERT IGNORE INTO `{maria_table}` ({col_list}) VALUES ({placeholders})"

            mc = maria.cursor()
            mc.execute("SET SESSION FOREIGN_KEY_CHECKS = 0")

            count = 0
            for batch_start in range(0, len(rows), BATCH_SIZE):
                batch = rows[batch_start:batch_start + BATCH_SIZE]
                values = []
                for row in batch:
                    vals = []
                    for i, v in enumerate(row):
                        if invert_flags[i]:
                            v = invert_value(v)
                        vals.append(v)
                    values.append(tuple(vals))
                mc.executemany(insert_sql, values)
                maria.commit()
                count += len(batch)

            mc.execute("SET SESSION FOREIGN_KEY_CHECKS = 1")
            mc.close()

            total += count
            print(f"  {pg_table} -> {maria_table} : {count} rows")

        except Exception as e:
            maria.rollback()
            print(f"  {pg_table} -> {maria_table} : FAILED ({e})")

    elapsed = time.time() - t0
    print(f"\nMigration complete: {total} total rows in {elapsed:.1f}s")
    pg.close()
    maria.close()


def main():
    p = argparse.ArgumentParser(description="MegaVoltsPP database migration tool")
    p.add_argument("--from", dest="from_db", required=True, choices=["mariadb", "postgresql", "postgres", "pg"])
    p.add_argument("--to", dest="to_db", required=True, choices=["mariadb", "postgresql", "postgres", "pg"])
    p.add_argument("--src-host", required=True)
    p.add_argument("--src-port", type=int, required=True)
    p.add_argument("--src-db", required=True)
    p.add_argument("--src-user", required=True)
    p.add_argument("--src-pass", required=True)
    p.add_argument("--dst-host", required=True)
    p.add_argument("--dst-port", type=int, required=True)
    p.add_argument("--dst-db", required=True)
    p.add_argument("--dst-user", required=True)
    p.add_argument("--dst-pass", required=True)
    args = p.parse_args()

    src = {"host": args.src_host, "port": args.src_port, "db": args.src_db, "user": args.src_user, "pass": args.src_pass}
    dst = {"host": args.dst_host, "port": args.dst_port, "db": args.dst_db, "user": args.dst_user, "pass": args.dst_pass}

    is_pg = lambda d: d in ("postgresql", "postgres", "pg")

    if args.from_db == "mariadb" and is_pg(args.to_db):
        print("Migrating MariaDB -> PostgreSQL\n")
        migrate_maria_to_pg(src, dst)
    elif is_pg(args.from_db) and args.to_db == "mariadb":
        print("Migrating PostgreSQL -> MariaDB\n")
        migrate_pg_to_maria(src, dst)
    else:
        print(f"Unsupported direction: {args.from_db} -> {args.to_db}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
