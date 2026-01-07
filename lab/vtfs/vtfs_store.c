#include "vtfs.h"
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>

static struct vtfs_fs *vtfs_fs(struct super_block *sb)
{
	return (struct vtfs_fs *)sb->s_fs_info;
}

static ino_t vtfs_next_ino(struct super_block *sb)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	return fs->next_ino++;
}

static struct vtfs_node *vtfs_node_alloc(struct super_block *sb,
                                         struct vtfs_node *parent,
                                         const char *name,
                                         umode_t mode)
{
	struct vtfs_node *n;

	n = kzalloc(sizeof(*n), GFP_KERNEL);
	if (!n)
		return NULL;

	n->name = kstrdup(name, GFP_KERNEL);
	if (!n->name) {
		kfree(n);
		return NULL;
	}

	n->mode = mode;
	n->ino = vtfs_next_ino(sb);
	n->parent = parent;

	INIT_LIST_HEAD(&n->children);
	INIT_LIST_HEAD(&n->siblings);

	return n;
}

static void vtfs_node_free_recursive(struct vtfs_node *n)
{
	struct vtfs_node *child, *tmp;

	list_for_each_entry_safe(child, tmp, &n->children, siblings) {
		list_del(&child->siblings);
		vtfs_node_free_recursive(child);
	}

	kfree(n->name);
	kfree(n);
}

int vtfs_store_init(struct super_block *sb)
{
	struct vtfs_fs *fs;

	fs = kzalloc(sizeof(*fs), GFP_KERNEL);
	if (!fs)
		return -ENOMEM;

	mutex_init(&fs->lock);
	fs->next_ino = 1001;

	/* выставляем сразу, иначе vtfs_next_ino() упадёт */
	sb->s_fs_info = fs;

	fs->root = vtfs_node_alloc(sb, NULL, "", S_IFDIR | 0777);
	if (!fs->root) {
		sb->s_fs_info = NULL;
		kfree(fs);
		return -ENOMEM;
	}

	/* фиксируем корень */
	fs->root->ino = 1000;
	fs->root->mode = S_IFDIR | 0777;
	fs->root->parent = NULL;

	return 0;
}

void vtfs_store_destroy(struct super_block *sb)
{
	struct vtfs_fs *fs = vtfs_fs(sb);

	if (!fs)
		return;

	mutex_lock(&fs->lock);
	if (fs->root) {
		vtfs_node_free_recursive(fs->root);
		fs->root = NULL;
	}
	mutex_unlock(&fs->lock);

	kfree(fs);
	sb->s_fs_info = NULL;
}

struct vtfs_node *vtfs_store_root(struct super_block *sb)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	return fs ? fs->root : NULL;
}

struct vtfs_node *vtfs_store_lookup(struct super_block *sb,
                                    struct vtfs_node *parent,
                                    const char *name)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	struct vtfs_node *child;

	if (!fs || !parent || !vtfs_is_dir(parent))
		return NULL;

	mutex_lock(&fs->lock);
	list_for_each_entry(child, &parent->children, siblings) {
		if (!strcmp(child->name, name)) {
			mutex_unlock(&fs->lock);
			return child;
		}
	}
	mutex_unlock(&fs->lock);

	return NULL;
}

struct vtfs_node *vtfs_store_create(struct super_block *sb,
                                    struct vtfs_node *parent,
                                    const char *name,
                                    umode_t mode)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	struct vtfs_node *child;

	if (!fs || !parent || !vtfs_is_dir(parent))
		return NULL;

	if (!name || !*name || !strcmp(name, ".") || !strcmp(name, ".."))
		return NULL;

	mutex_lock(&fs->lock);

	list_for_each_entry(child, &parent->children, siblings) {
		if (!strcmp(child->name, name)) {
			mutex_unlock(&fs->lock);
			return NULL; /* exists */
		}
	}

	child = vtfs_node_alloc(sb, parent, name, mode);
	if (!child) {
		mutex_unlock(&fs->lock);
		return NULL;
	}

	list_add_tail(&child->siblings, &parent->children);
	mutex_unlock(&fs->lock);

	return child;
}

int vtfs_store_unlink(struct super_block *sb,
                      struct vtfs_node *parent,
                      const char *name)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	struct vtfs_node *child, *tmp;

	if (!fs || !parent || !vtfs_is_dir(parent))
		return -ENOENT;

	mutex_lock(&fs->lock);
	list_for_each_entry_safe(child, tmp, &parent->children, siblings) {
		if (!strcmp(child->name, name)) {
			if (vtfs_is_dir(child)) {
				mutex_unlock(&fs->lock);
				return -EISDIR;
			}
			list_del(&child->siblings);
			vtfs_node_free_recursive(child);
			mutex_unlock(&fs->lock);
			return 0;
		}
	}
	mutex_unlock(&fs->lock);

	return -ENOENT;
}

int vtfs_store_rmdir(struct super_block *sb,
                     struct vtfs_node *parent,
                     const char *name)
{
	struct vtfs_fs *fs = vtfs_fs(sb);
	struct vtfs_node *child, *tmp;

	if (!fs || !parent || !vtfs_is_dir(parent))
		return -ENOENT;

	mutex_lock(&fs->lock);
	list_for_each_entry_safe(child, tmp, &parent->children, siblings) {
		if (!strcmp(child->name, name)) {
			if (!vtfs_is_dir(child)) {
				mutex_unlock(&fs->lock);
				return -ENOTDIR;
			}
			if (!list_empty(&child->children)) {
				mutex_unlock(&fs->lock);
				return -ENOTEMPTY;
			}
			list_del(&child->siblings);
			vtfs_node_free_recursive(child);
			mutex_unlock(&fs->lock);
			return 0;
		}
	}
	mutex_unlock(&fs->lock);

	return -ENOENT;
}