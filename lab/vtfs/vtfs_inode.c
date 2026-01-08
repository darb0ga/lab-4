#include "vtfs.h"
#include <linux/time.h>

static umode_t vtfs_force_mode(umode_t mode)
{
	return (mode & S_IFMT) | 0777;
}

struct inode *vtfs_inode_from_node(struct super_block *sb, struct vtfs_node *node)
{
	struct inode *inode;
	umode_t mode;

	if (!node)
		return NULL;

	inode = new_inode(sb);
	if (!inode)
		return NULL;

	mode = vtfs_force_mode(node->mode);
	node->mode = mode;

	inode_init_owner(&nop_mnt_idmap, inode, NULL, mode);
	inode->i_mode = mode;

	inode->i_ino = node->ino;
	inode_set_ctime_current(inode);

	inode->i_private = node;

	if (S_ISDIR(mode)) {
		inode->i_op  = &vtfs_dir_iops;
		inode->i_fop = &vtfs_dir_fops;
		set_nlink(inode, 2);
	} else {
		inode->i_fop = &vtfs_file_fops;
		set_nlink(inode, 1);
	}

	return inode;
}
int vtfs_link(struct dentry *old_dentry,
              struct inode *parent_dir,
              struct dentry *new_dentry)
{
	struct inode *inode = d_inode(old_dentry);
	struct vtfs_node *node = inode->i_private;
	struct vtfs_node *parent = parent_dir->i_private;

	if (!node || !parent)
		return -EINVAL;

	if (!S_ISREG(node->mode))
		return -EPERM;

	mutex_lock(&vtfs_fs(parent_dir->i_sb)->lock);

	inc_nlink(inode);
	ihold(inode);
	d_add(new_dentry, inode);

	mutex_unlock(&vtfs_fs(parent_dir->i_sb)->lock);

	return 0;
}