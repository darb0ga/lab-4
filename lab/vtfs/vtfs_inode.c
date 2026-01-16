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

	if (S_ISDIR(mode)) {
		inode->i_private = node;
		inode->i_op  = &vtfs_dir_iops;
		inode->i_fop = &vtfs_dir_fops;
		set_nlink(inode, 2);
	} else {
		inode->i_private = node->f;     
		inode->i_fop = &vtfs_file_fops;
		set_nlink(inode, 1);
	}


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
    struct super_block *sb = parent_dir->i_sb;
    struct vtfs_node *parent = parent_dir->i_private;
    struct vtfs_node *target;
    int ret;

    if (!parent)
        return -EINVAL;

    target = vtfs_store_lookup(sb, parent, old_dentry->d_name.name);
    if (!target)
        return -ENOENT;

    if (!S_ISREG(target->mode))
        return -EPERM;

    ret = vtfs_store_link(sb, parent, new_dentry->d_name.name, target);
    if (ret)
        return ret;

    inc_nlink(d_inode(old_dentry));
    ihold(d_inode(old_dentry));
    d_add(new_dentry, d_inode(old_dentry));

    return 0;
}