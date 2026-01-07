#ifndef VTFS_H
#define VTFS_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/list.h>


struct vtfs_node {
	char *name;
	umode_t mode;
	ino_t ino;

	struct vtfs_node *parent;

	struct list_head children;
	struct list_head siblings;
};

struct vtfs_fs {
	struct vtfs_node *root;
	ino_t next_ino;
	struct mutex lock;
};

static inline bool vtfs_is_dir(const struct vtfs_node *n)
{
	return S_ISDIR(n->mode);
}

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

extern struct file_system_type vtfs_fs_type;

extern const struct super_operations vtfs_super_ops;

extern const struct inode_operations vtfs_dir_iops;
extern const struct inode_operations vtfs_file_iops;

extern const struct file_operations vtfs_dir_fops;
extern const struct file_operations vtfs_file_fops;

struct inode *vtfs_inode_from_node(struct super_block *sb,
                                   struct vtfs_node *node);

int vtfs_fill_super(struct super_block *sb, void *data, int silent);

#endif