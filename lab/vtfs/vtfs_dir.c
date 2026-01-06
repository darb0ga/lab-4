#include "vtfs.h"
#include <linux/fs.h>
#include <linux/string.h>

/*
 * Lookup file in directory
 */
static struct dentry *vtfs_lookup(struct inode *dir,
                                  struct dentry *dentry,
                                  unsigned int flags)
{
	struct vtfs_node *parent = dir->i_private;
	struct vtfs_node *node;
	struct inode *inode = NULL;

	node = vtfs_store_lookup(dir->i_sb, parent,
	                         dentry->d_name.name);
	if (node)
		inode = vtfs_make_inode(dir->i_sb, node);

	d_add(dentry, inode);
	return NULL;
}

/*
 * Create file
 */
static int vtfs_create(struct mnt_idmap *idmap,
                       struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
	struct vtfs_node *parent = dir->i_private;
	struct vtfs_node *node;
	struct inode *inode;

	node = vtfs_store_create(dir->i_sb, parent,
	                         dentry->d_name.name,
	                         S_IFREG | mode);
	if (!node)
		return -ENOMEM;

	inode = vtfs_make_inode(dir->i_sb, node);
	if (!inode)
		return -ENOMEM;

	d_add(dentry, inode);
	return 0;
}

/*
 * Remove file
 */
static int vtfs_unlink(struct inode *dir, struct dentry *dentry)
{
	struct vtfs_node *parent = dir->i_private;

	return vtfs_store_unlink(dir->i_sb, parent,
	                         dentry->d_name.name);
}

/*
 * Create directory
 */
static int vtfs_mkdir(struct mnt_idmap *idmap,
                      struct inode *dir,
                      struct dentry *dentry,
                      umode_t mode)
{
	struct vtfs_node *parent = dir->i_private;
	struct vtfs_node *node;
	struct inode *inode;

	node = vtfs_store_create(dir->i_sb, parent,
	                         dentry->d_name.name,
	                         S_IFDIR | mode);
	if (!node)
		return -ENOMEM;

	inode = vtfs_make_inode(dir->i_sb, node);
	if (!inode)
		return -ENOMEM;

	d_add(dentry, inode);
	return 0;
}

/*
 * Remove directory
 */
static int vtfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct vtfs_node *parent = dir->i_private;

	return vtfs_store_rmdir(dir->i_sb, parent,
	                        dentry->d_name.name);
}

/*
 * Iterate directory
 */
static int vtfs_iterate(struct file *file,
                        struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	struct vtfs_node *node = inode->i_private;
	struct vtfs_node *child;

	if (!dir_emit_dots(file, ctx))
		return 0;

	list_for_each_entry(child, &node->children, siblings) {
		if (!dir_emit(ctx,
		               child->name,
		               strlen(child->name),
		               child->ino,
		               vtfs_is_dir(child) ? DT_DIR : DT_REG))
			return 0;
		ctx->pos++;
	}

	return 0;
}

/*
 * inode / file operations
 */
struct inode_operations vtfs_inode_ops = {
	.lookup = vtfs_lookup,
	.create = vtfs_create,
	.unlink = vtfs_unlink,
	.mkdir  = vtfs_mkdir,
	.rmdir  = vtfs_rmdir,
};

struct file_operations vtfs_dir_ops = {
	.owner  = THIS_MODULE,
	.iterate_shared = vtfs_iterate,
};

struct file_operations vtfs_file_ops = {
	.owner = THIS_MODULE,
};
