#include "vtfs.h"
#include <linux/string.h>
#include <linux/errno.h>

static struct vtfs_fs *vtfs_fs(struct super_block *sb)
{
	return (struct vtfs_fs *)sb->s_fs_info;
}


static struct dentry *vtfs_lookup(struct inode *dir_inode,
                                  struct dentry *dentry,
                                  unsigned int flags)
{
	struct super_block *sb = dir_inode->i_sb;
	struct vtfs_node *dir = dir_inode->i_private;
	struct vtfs_node *child;
	struct inode *inode;

	(void)flags;

	if (!dir || !vtfs_is_dir(dir))
		return NULL;

	child = vtfs_store_lookup(sb, dir, dentry->d_name.name);
	if (!child)
		return NULL;

	inode = vtfs_inode_from_node(sb, child);
	if (!inode)
		return ERR_PTR(-ENOMEM);

	d_add(dentry, inode);
	return NULL;
}


static int vtfs_iterate(struct file *file, struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	struct vtfs_fs *fs = vtfs_fs(sb);
	struct vtfs_node *dir = inode->i_private;
	struct vtfs_node *child;

	loff_t idx = 0;
	loff_t want;

	if (!fs || !dir || !vtfs_is_dir(dir))
		return 0;

	if (!dir_emit_dots(file, ctx))
		return 0;

	want = ctx->pos - 2;

	mutex_lock(&fs->lock);

	list_for_each_entry(child, &dir->children, siblings) {
		if (idx++ < want)
			continue;

		if (!dir_emit(ctx,
		              child->name,
		              strlen(child->name),
		              child->ino,
		              vtfs_is_dir(child) ? DT_DIR : DT_REG)) {
			mutex_unlock(&fs->lock);
			return 0;
		}
		ctx->pos++; 
	}

	mutex_unlock(&fs->lock);
	return 0;
}


static int vtfs_create(struct mnt_idmap *idmap,
                       struct inode *dir_inode,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
	struct super_block *sb = dir_inode->i_sb;
	struct vtfs_node *dir = dir_inode->i_private;
	struct vtfs_node *node;
	struct inode *inode;

	(void)idmap;
	(void)excl;

	if (!dir || !vtfs_is_dir(dir))
		return -ENOTDIR;

	mode = S_IFREG | 0777; /* форсим как в ТЗ */

	node = vtfs_store_create(sb, dir, dentry->d_name.name, mode);
	if (!node)
		return -EEXIST;

	inode = vtfs_inode_from_node(sb, node);
	if (!inode)
		return -ENOMEM;

	d_add(dentry, inode);
	return 0;
}

static int vtfs_unlink(struct inode *dir_inode, struct dentry *dentry)
{
	struct super_block *sb = dir_inode->i_sb;
	struct vtfs_node *dir = dir_inode->i_private;

	if (!dir || !vtfs_is_dir(dir))
		return -ENOTDIR;

	return vtfs_store_unlink(sb, dir, dentry->d_name.name);
}

static int vtfs_mkdir(struct mnt_idmap *idmap,
                      struct inode *dir_inode,
                      struct dentry *dentry,
                      umode_t mode)
{
	struct super_block *sb = dir_inode->i_sb;
	struct vtfs_node *dir = dir_inode->i_private;
	struct vtfs_node *node;
	struct inode *inode;

	(void)idmap;

	if (!dir || !vtfs_is_dir(dir))
		return -ENOTDIR;

	mode = S_IFDIR | 0777;

	node = vtfs_store_create(sb, dir, dentry->d_name.name, mode);
	if (!node)
		return -EEXIST;

	inode = vtfs_inode_from_node(sb, node);
	if (!inode)
		return -ENOMEM;

	inode_inc_link_count(dir_inode);

	d_add(dentry, inode);
	return 0;
}

static int vtfs_rmdir(struct inode *dir_inode, struct dentry *dentry)
{
	struct super_block *sb = dir_inode->i_sb;
	struct vtfs_node *dir = dir_inode->i_private;
	int ret;

	if (!dir || !vtfs_is_dir(dir))
		return -ENOTDIR;

	ret = vtfs_store_rmdir(sb, dir, dentry->d_name.name);
	if (ret == 0)
		inode_dec_link_count(dir_inode);

	return ret;
}

const struct inode_operations vtfs_dir_iops = {
	.lookup = vtfs_lookup,
	.create = vtfs_create,
	.unlink = vtfs_unlink,
	.mkdir  = vtfs_mkdir,
	.rmdir  = vtfs_rmdir,
};

const struct file_operations vtfs_dir_fops = {
	.owner = THIS_MODULE,
	.iterate_shared = vtfs_iterate,
	.llseek = generic_file_llseek,
};