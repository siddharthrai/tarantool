#ifndef TARANTOOL_BOX_ENGINE_H_INCLUDED
#define TARANTOOL_BOX_ENGINE_H_INCLUDED
/*
 * Copyright 2010-2016, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdint.h>
#include <small/rlist.h>

#include "error.h"

struct engine;
struct txn;
struct txn_stmt;
struct space;
struct space_def;
struct vclock;
struct xstream;

extern struct rlist engines;

typedef int
engine_backup_cb(const char *path, void *arg);

#if defined(__cplusplus)

struct engine_vtab {
	/** Destroy an engine instance. */
	void (*shutdown)(struct engine *);
	/** Allocate a new space instance. */
	struct space *(*create_space)(struct engine *engine,
			struct space_def *def, struct rlist *key_list);
	/**
	 * Write statements stored in checkpoint @vclock to @stream.
	 */
	void (*join)(struct engine *engine, struct vclock *vclock,
		     struct xstream *stream);
	/**
	 * Begin a new single or multi-statement transaction.
	 * Called on first statement in a transaction, not when
	 * a user said begin(). Effectively it means that
	 * transaction in the engine begins with the first
	 * statement.
	 */
	void (*begin)(struct engine *, struct txn *);
	/**
	 * Begine one statement in existing transaction.
	 */
	void (*begin_statement)(struct engine *, struct txn *);
	/**
	 * Called before a WAL write is made to prepare
	 * a transaction for commit in the engine.
	 */
	void (*prepare)(struct engine *, struct txn *);
	/**
	 * End the transaction in the engine, the transaction
	 * has been successfully written to the WAL.
	 * This method can't throw: if any error happens here,
	 * there is no better option than panic.
	 */
	void (*commit)(struct engine *, struct txn *);
	/*
	 * Called to roll back effects of a statement if an
	 * error happens, e.g., in a trigger.
	 */
	void (*rollback_statement)(struct engine *, struct txn *,
				   struct txn_stmt *);
	/*
	 * Roll back and end the transaction in the engine.
	 */
	void (*rollback)(struct engine *, struct txn *);
	/**
	 * Bootstrap an empty data directory
	 */
	void (*bootstrap)(struct engine *);
	/**
	 * Begin initial recovery from checkpoint or dirty disk data.
	 * On local recovery @recovery_vclock points to the vclock
	 * used for assigning LSNs to statements replayed from WAL.
	 * On remote recovery, it is set to NULL.
	 */
	void (*begin_initial_recovery)(struct engine *engine,
			const struct vclock *recovery_vclock);
	/**
	 * Notify engine about a start of recovering from WALs
	 * that could be local WALs during local recovery
	 * of WAL catch up durin join on slave side
	 */
	void (*begin_final_recovery)(struct engine *);
	/**
	 * Inform the engine about the end of recovery from the
	 * binary log.
	 */
	void (*end_recovery)(struct engine *);
	/**
	 * Begin a two-phase checkpoint creation in this
	 * engine (snapshot is a memtx idea of a checkpoint).
	 * Must not yield.
	 */
	int (*begin_checkpoint)(struct engine *);
	/**
	 * Wait for a checkpoint to complete.
	 */
	int (*wait_checkpoint)(struct engine *, struct vclock *);
	/**
	 * All engines prepared their checkpoints,
	 * fix up the changes.
	 */
	void (*commit_checkpoint)(struct engine *, struct vclock *);
	/**
	 * An error in one of the engines, abort checkpoint.
	 */
	void (*abort_checkpoint)(struct engine *);
	/**
	 * Remove files that are not needed to recover
	 * from checkpoint with @lsn or newer.
	 *
	 * If this function returns a non-zero value, garbage
	 * collection is aborted, i.e. this method isn't called
	 * for other engines and xlog files aren't deleted.
	 *
	 * Used to abort garbage collection in case memtx engine
	 * fails to delete a snapshot file, because we recover
	 * checkpoint list by scanning the snapshot directory.
	 */
	int (*collect_garbage)(struct engine *engine, int64_t lsn);
	/**
	 * Backup callback. It is supposed to call @cb for each file
	 * that needs to be backed up in order to restore from the
	 * checkpoint @vclock.
	 */
	int (*backup)(struct engine *engine, struct vclock *vclock,
		      engine_backup_cb cb, void *cb_arg);
	/**
	 * Check definition of a new space for engine-specific
	 * limitations. E.g. not all engines support temporary
	 * tables.
	 */
	void (*check_space_def)(struct space_def *);
};

struct engine {
	/** Virtual function table. */
	const struct engine_vtab *vtab;
	/** Engine name. */
	const char *name;
	/** Engine id. */
	uint32_t id;
	/** Used for search for engine by name. */
	struct rlist link;
};

/** Register engine engine instance. */
void engine_register(struct engine *engine);

/** Call a visitor function on every registered engine. */
#define engine_foreach(engine) rlist_foreach_entry(engine, &engines, link)

/** Find engine engine by name. */
struct engine *
engine_by_name(const char *name);

/** Find engine by name and raise error if not found. */
static inline struct engine *
engine_find(const char *name)
{
	struct engine *engine = engine_by_name(name);
	if (engine == NULL)
		tnt_raise(LoggedError, ER_NO_SUCH_ENGINE, name);
	return engine;
}

static inline struct space *
engine_create_space(struct engine *engine, struct space_def *def,
		    struct rlist *key_list)
{
	return engine->vtab->create_space(engine, def, key_list);
}

static inline void
engine_begin(struct engine *engine, struct txn *txn)
{
	engine->vtab->begin(engine, txn);
}

static inline void
engine_begin_statement(struct engine *engine, struct txn *txn)
{
	engine->vtab->begin_statement(engine, txn);
}

static inline void
engine_prepare(struct engine *engine, struct txn *txn)
{
	engine->vtab->prepare(engine, txn);
}

static inline void
engine_commit(struct engine *engine, struct txn *txn)
{
	engine->vtab->commit(engine, txn);
}

static inline void
engine_rollback_statement(struct engine *engine, struct txn *txn,
			  struct txn_stmt *stmt)
{
	engine->vtab->rollback_statement(engine, txn, stmt);
}

static inline void
engine_rollback(struct engine *engine, struct txn *txn)
{
	engine->vtab->rollback(engine, txn);
}

static inline void
engine_check_space_def(struct engine *engine, struct space_def *def)
{
	engine->vtab->check_space_def(def);
}

/** Shutdown all engine factories. */
void engine_shutdown();

/**
 * Initialize an empty data directory
 */
void
engine_bootstrap();

/**
 * Called at the start of recovery.
 */
void
engine_begin_initial_recovery(const struct vclock *recovery_vclock);

/**
 * Called in the middle of JOIN stage,
 * when xlog catch-up process is started
 */
void
engine_begin_final_recovery();

/**
 * Called at the end of recovery.
 * Build secondary keys in all spaces.
 */
void
engine_end_recovery();

/**
 * Feed checkpoint data as join events to the replicas.
 * (called on the master).
 */
void
engine_join(struct vclock *vclock, struct xstream *stream);

extern "C" {
#endif /* defined(__cplusplus) */

int
engine_begin_checkpoint();

/**
 * Create a checkpoint.
 */
int
engine_commit_checkpoint(struct vclock *vclock);

void
engine_abort_checkpoint();

int
engine_collect_garbage(int64_t lsn);

int
engine_backup(struct vclock *vclock, engine_backup_cb cb, void *cb_arg);

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* TARANTOOL_BOX_ENGINE_H_INCLUDED */
