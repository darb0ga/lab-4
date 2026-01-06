#pragma once

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/mutex.h>
#include <linux/list.h>

#define VTFS_NAME "vtfs"

extern struct inode_operations vtfs_inode_ops;
extern struct file_operations  vtfs_dir_ops;
extern struct file_operations  vtfs_file_ops;


struct vtfs_node {
	char *name;
	unsigned long ino;
	umode_t mode;              /* contains type + perms */
	struct vtfs_node *parent;

	struct list_head siblings; /* entry in parent's children list */
	struct list_head children; /* list of vtfs_node::siblings */
};

/* filesystem runtime state (per mount) */
struct vtfs_fs {
	struct mutex lock;
	struct vtfs_node *root;
	unsigned long next_ino;
};

static inline bool vtfs_is_dir(const struct vtfs_node *n)
{
	return S_ISDIR(n->mode);
}

static inline bool vtfs_is_reg(const struct vtfs_node *n)
{
	return S_ISREG(n->mode);
}

/* ===== exported symbols ===== */

extern struct file_system_type vtfs_fs_type;

/* superblock lifecycle */
struct dentry *vtfs_mount(struct file_system_type *fs_type,
                          int flags,
                          const char *token,
                          void *data);

void vtfs_kill_sb(struct super_block *sb);

int vtfs_fill_super(struct super_block *sb, void *data, int silent);

/* inode ops init */
void vtfs_init_dir_inode(struct inode *inode);
void vtfs_init_file_inode(struct inode *inode);

/* inode creation helper */
struct inode *vtfs_make_inode(struct super_block *sb,
                              struct vtfs_node *node);

/* RAM store API */
int vtfs_store_init(struct super_block *sb);
void vtfs_store_destroy(struct super_block *sb);

struct vtfs_node *vtfs_store_root(struct super_block *sb);

struct vtfs_node *vtfs_store_lookup(struct super_block *sb,
                                    struct vtfs_node *parent,
                                    const char *name);

struct vtfs_node *vtfs_store_create(struct super_block *sb,
                                    struct vtfs_node *parent,
                                    const char *name,
                                    umode_t mode);

int vtfs_store_unlink(struct super_block *sb,
                      struct vtfs_node *parent,
                      const char *name);

int vtfs_store_rmdir(struct super_block *sb,
                     struct vtfs_node *parent,
                     const char *name);